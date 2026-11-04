# The kernel as a C++20 module

The kernel is a single named module, `Cryptix`. There are no kernel headers any more: every
former header is an *interface partition*, and every former `.cpp` is an *implementation
partition*.

```
Kernel/Cryptix.cppm                       export module Cryptix;  -- re-exports every partition
Kernel/Scheduler/Thread.cppm              export module Cryptix:Scheduler.Thread;
Kernel/Scheduler/Thread.cpp               module Cryptix:Impl.Scheduler.Thread;
```

Building it needs CMake >= 3.28 with the Ninja generator and `clang-scan-deps` (clang >= 17);
see the README.

## Why one module?

The old headers formed one big dependency cycle (Arch, Memory, Scheduler, VFS, Locking, ...)
that was cut with forward declarations (`class Thread;`). A forward declaration only refers to
the real class when both belong to the *same module*, so the cyclic part cannot be split into
several modules without redesigning it. Inside one module the partitions import each other
along the old `#include` graph (which was acyclic), and the forward declarations keep working.

## Anatomy of a partition

```cpp
/* license */
module;                                   // global module fragment: everything that is not a module

#include <Common.hpp>                     // macro headers
#include <Prism/Containers/Vector.hpp>    // Prism, fmt, magic_enum, uACPI, libstdc++, C headers

export module Cryptix:VFS.INode;          // the module declaration

export import :VFS.DirectoryEntry;        // what the header used to #include
import :Library.Locking.Spinlock;         // plain `import`: needed here, not re-exported

export class INode { ... };               // declarations are exported
```

* **Imports mirror includes.** A partition `export import`s the partitions it used to include, so
  including (= importing) it still provides everything it provided before.
* **Translation units import what they included.** `.cpp` files are *implementation partitions*
  (`module Cryptix:Impl.X;`) instead of `module Cryptix;` units on purpose: the latter would
  implicitly import the primary interface and with it *every* partition, so unrelated names such
  as the `API::Process` namespace and the `Process` class would suddenly collide.
* **Only the global module fragment sees non-module code.** Names from `#include`d headers are
  not visible through an `import`, so each unit lists the external headers it uses.

## What is still a header, and why

| Header | Reason |
| ------ | ------ |
| `Compiler.hpp`, `Common.hpp`, `Library/Logger.hpp`, `Debug/Assertions.hpp`, `Library/Module.hpp`, `API/Syscall.hpp`, ... | **Macros cannot be exported.** Headers that define macros are split: the macros stay in the `.hpp`, the declarations live in a partition with the same name (`Logger.hpp` + `Logger.cppm`). Include the `.hpp` in the global module fragment *and* import the partition. |
| `API/Posix/**`, `Library/KLibC/*.h` | C headers (Linux UAPI, the kernel's libc). |
| `Library/KLibC++/*` | The freestanding replacements for standard library headers. |
| `API/Limits.hpp`, `API/UnixTypes.hpp` | Included by the C headers above and by Prism. |
| `Library/Stacktrace.hpp` | Empty. Prism (a separate repository) still includes it. |
| generated `Config.hpp`, `Debug/Debug.hpp` | Macro-only configuration. |

## Rules that come with modules

* **Do not redeclare an entity of the global module.** A `class Foo;` or `struct stat` in a
  partition declares a *new* module-attached entity if `Foo` came from a header; include the
  header instead. (`struct sockaddr* p` is written `sockaddr* p` for this reason.)
* **Definitions that non-module code calls must be global-module entities.** Prism, the
  demangler, uACPI and the C++ runtime know nothing about `Cryptix`; names defined in the module
  are mangled with the module name. Such entry points are defined in plain translation units
  and forward into the module, usually through `extern "C"`:
  `Library/PrismHooks.cpp` (`panic`, `Logger::Print`, `std::terminate`),
  `Memory/Allocator/Operators.cpp` (`operator new/delete`) and
  `Memory/HigherHalfOffset.cpp` (`MM::HigherHalfOffset`).
* **Inline functions must be defined where they are declared.** Declare `inline` only what the
  partition defines.
* **Mutually dependent types need to be complete in one translation unit.** `INode` and
  `DirectoryEntry` live in one partition (`VFS.INode`); a constructor that has to destroy a
  `Ref<T>` of a type that is only forward declared belongs in the `.cpp`.
* **Define the constructors of polymorphic classes next to their key function.** clang 18 only
  emits the vtable of a class imported from a module in the translation unit that defines the
  key function when a constructor is defined there too.
* **No `__DATE__`/`__TIME__` in a module** (the build date comes from CMake). **Partition names
  are macro-expanded**, so none of their components may be a macro (`Panic` is one; the partition
  is `Debug.KernelPanic`).
* **`static` at namespace scope cannot be exported.** Use `inline` (or `constexpr`) instead.

## Adding code

* New header-like code: create `Foo.cppm` (`export module Cryptix:Dir.Foo;`) and add it to the
  `FILE_SET CXX_MODULES` list in the directory's `CMakeLists.txt` and an `export import` line to
  `Kernel/Cryptix.cppm`.
* New source file: `module Cryptix:Impl.Dir.Foo;`, import the partitions it needs, and add it to
  the same `CXX_MODULES` list (CMake requires every unit that declares a partition there).

## Loadable modules

Modules under `Modules/` are built as separate objects and import the kernel (`import Cryptix;`).
A module with headers of its own is a named module as well (`Cryptix.Modules.Serio`,
`Cryptix.Modules.AtKeyboard`, ...): its primary interface replaces the old header, and `main.cpp`
is the implementation unit. Because the kernel and the modules share the module interface they
have to be compiled with the same target features; only the code model differs.
