/*
 * Created by v1tr10l7 on 04.03.2025.
 *
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Library/Module.hpp>
#include <Compiler.hpp>
#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Library/Logger.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <Prism/String/String.hpp>
#include <compare>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/Containers/RedBlackTree.hpp>
#include <Prism/Core/Error.hpp>
#include <Prism/Memory/Buffer.hpp>
#include <Prism/Memory/Memory.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/Utility/Delegate.hpp>
#include <Prism/Utility/PathView.hpp>
#include <API/UnixTypes.hpp>
#include <Arch/Arch.hpp>
#include <API/Posix/time.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>
#include <Prism/Utility/Atomic.hpp>
#if defined(CTOS_TARGET_X86_64)
#include <API/Posix/termios.h>
#include <Arch/x86_64/CPUIntrinsics.hpp>
#endif
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Containers/Span.hpp>
#include <Prism/Containers/KeyValuePair.hpp>
#include <Prism/Utility/Math.hpp>
#include <Prism/Containers/IntrusiveRefList.hpp>
#include <Prism/Utility/Time.hpp>
#if !(defined(CTOS_TARGET_X86_64)) && defined(CTOS_TARGET_AARCH64)
#include <Arch/aarch64/CPUIntrinsics.hpp>
#endif
#include <Prism/Utility/Path.hpp>
#include <API/Posix/sys/stat.h>
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Memory/WeakRef.hpp>
#include <uacpi/resources.h>
#include <uacpi/utilities.h>

export module Cryptix:Library.Module;

export import :Library.ELF.Image;

export namespace ELF
{
    class Image;
}

export using ModuleInitProc      = bool (*)();
export using ModuleTerminateProc = void (*)();
export struct CTOS_PACKED_ALIGNED(8) ModulePreludium
{
    const char*         Name;

    ModuleInitProc      Initialize;
    ModuleTerminateProc Terminate;
};

export struct ModuleInformation
{
    String Author;
    String Description;
    String License;
    String Version;
};

export enum class ModuleParamType : u8
{
    eInt    = 1,
    eBool   = 2,
    eString = 3,
};

export struct ModuleParameter
{
    const char*     Name;
    ModuleParamType Type;
    // Pointer to the actual variable in the module
    void*           Address;
    const char*     Description;
};

export using InitArrayEntry = void (*)();
export using FiniArrayEntry = void (*)();

export enum class ModuleState
{
    eEmpty    = 0x00,
    eLoaded   = 0x01,
    eReady    = 0x02,
    eRunning  = 0x03,
    eUnloaded = 0x04,
};
export struct Module : public RefCounted
{
    static Module* s_ThisModule;
    constexpr Module() { s_ThisModule = this; }
    static Module* ThisModule() { return s_ThisModule; }

    friend class IntrusiveRefList<Module>;
    friend struct IntrusiveRefListHook<Module>;

    using List = IntrusiveRefList<Module>;

    String                                  Name;
    ModuleState                             State = ModuleState::eEmpty;

    ModuleInformation                       Info;
    UnorderedMap<StringView, ::Ref<Module>> Dependencies;

    Span<InitArrayEntry, DynamicExtent>     InitArray;
    Span<InitArrayEntry, DynamicExtent>     FiniArray;

    bool                                    Initialized;
    bool                                    Failed;

    IntrusiveRefListHook<Module>            Hook;

    ModulePreludium*                        Preludium = nullptr;
    ::Ref<ELF::Image>                       Image     = nullptr;

    ModuleInitProc                          Initialize;
    ModuleTerminateProc                     Terminate;
    Span<ModuleParameter, DynamicExtent>    Parameters;

    void                                    ParseModuleInfo();
    void                                    ResolveParameters();

    void                                    Prepare();
    ErrorOr<void>                           Dispatch();
    void                                    Unload();

    static bool                             Load();
};
