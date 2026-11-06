# 🗺️ CryptixOS Roadmap (0.1 → 1.0)

This is the version-by-version plan for CryptixOS. After a first version that makes everything
that exists today reliable, it is built around **milestones you can show off**: every version
ends with a demo you can screenshot, record, or post. The essential work (stability,
correctness, security) is not skipped. It is scheduled *alongside* the demos, so the base never
rots underneath them.

0.1 is a plain checklist with exit criteria. From 0.2 on, each version has:

- **Headline**: the demo that marks the version as done. It is split into small stages, so you
  get a win every few weeks instead of one at the very end.
- **What the demo needs**: the kernel and userland features the headline depends on.
- **Floor**: stability and completeness work that must land in the *same* version, even though
  the demo doesn't strictly need it.
- **Exit criteria**: the demo works *and* the floor holds.

It is a plan, not a schedule. A version ships when it is *done*, not on a date.

**How to read the checkboxes:**
- `[x]`: done and working.
- `[ ]`: not done yet.
- `[ ]` 🚧: work in progress. It exists in the tree but is incomplete.

Userland components have their own detailed roadmaps:
- [Aurora (init system) roadmap](Documentation/CryptixInit-RoadMap.md)
- [Awesome Shell (awsh) roadmap](Documentation/AwesomeShell-RoadMap.md)

## Contents

- [The milestone ladder](#the-milestone-ladder)
- [Where we are: 0.0.x](#where-we-are-00x)
- [0.1: Stable Core](#01-stable-core)
- [0.2: DOOM & Terminal Life](#02-doom--terminal-life)
- [0.3: Online, and nmap](#03-online-and-nmap)
- [0.4: X11](#04-x11)
- [0.5: Real Memory Management](#05-real-memory-management)
- [0.6: Your Data Survives](#06-your-data-survives)
- [0.7: Real Hardware](#07-real-hardware)
- [0.8: Multi-User & Hardened](#08-multi-user--hardened)
- [0.9: Self-Hosting, aarch64 & Beta](#09-self-hosting-aarch64--beta)
- [1.0: First Stable Release](#10-first-stable-release)
- [Side quests](#side-quests)
- [Hall of fame](#hall-of-fame)
- [Must-Have Before 1.0](#must-have-before-10)
- [How to use this roadmap](#how-to-use-this-roadmap)

---

## The milestone ladder

The short version, if you only read one section:

| Version | Headline demo | Unlocked by |
|---|---|---|
| **0.1** | A shell session survives 1 hour of scripted use | process, signal, fd and TTY correctness, CI |
| **0.2** | Play DOOM E1M1, then tmux with htop and vim side by side | fb `mmap`, evdev, PTYs, job control, `poll` |
| **0.3** | `ping`, `curl https://…`, then **`nmap`** a real host | TCP/IP stack, NIC drivers, raw sockets |
| **0.4** | Xorg + twm + xterm + xeyes, then **DOOM in an X window** | evdev mouse, shared `mmap`, `dlopen`, VT ioctls |
| **0.5** | `gcc hello.c && ./a.out` *on* CryptixOS, Python scripts | COW fork, page cache, futexes |
| **0.6** | `git clone` over HTTPS, build it, reboot, `e2fsck` clean | buffer cache, AHCI/virtio-blk, VFS semantics |
| **0.7** | Boot your laptop from USB, play DOOM *with sound* | xHCI, USB HID, HDA audio, mode setting |
| **0.8** | `ssh` into CryptixOS as a normal user who can't break it | credentials, PAM, SMEP/SMAP, fuzzing |
| **0.9** | CryptixOS builds its own kernel, and it boots | native toolchain, aarch64 parity |
| **1.0** | Install it on a disk and use it | installer, release process |

Graphics comes at **0.4**, not at the end. Networking comes at **0.3**. The deep VM, storage and
security work comes later, when there are real programs (X, gcc, git) to stress it.

Every feature from the original 0.1 → 1.0 plan is still here. Some moved to a different
version, and none were dropped.

---

## Where we are: 0.0.x

CryptixOS boots on x86_64 (UEFI and BIOS, via Limine), and aarch64 boots at an early stage.
It runs a real userland on top of mlibc:
- bash, dash and zsh;
- coreutils, vim, nano and less;
- the Aurora init system and the awsh shell;
- already packaged: `nyancat`, `sl`, `cmatrix`, `neofetch`, `portscanner`, and a
  `doom-generic` recipe.

The kernel already has:
- SMP scheduling and signals;
- a VFS with ext2, FAT32, tmpfs, procfs, devtmpfs and initrd;
- NVMe, PCI/PCIe, ACPI (uACPI) and HPET/LAPIC timers;
- a framebuffer device with the basic `FBIOGET_*` ioctls;
- an RTL8139 driver (basic);
- loadable kernel modules;
- the first pieces of PTYs and sockets.

Core paths still have correctness bugs. Recent fixes include:
- threads disappearing after returning from a signal handler;
- process teardown freeing memory that was still in use;
- a deadlock when logging from interrupt context;
- incorrect file-descriptor semantics.

**Stability comes before new features.** That is what 0.1 is for.

---

## 0.1: Stable Core

> **Theme:** everything that exists today must work *every* time. A shell session must never
> randomly die, hang or corrupt memory.

### Process lifecycle

- [x] **`fork` / `execve` / `exit` / `waitpid`**
  A parent can create children, replace their image, and collect their exit status. Zombies
  stay until the parent reaps them.
- [x] **Deferred process teardown**
  A process is freed only after it has been reaped *and* its last thread has left the CPU.
  This prevents freeing a kernel stack the CPU is still running on.
- [ ] **Multi-threaded teardown**
  Track off-CPU state for *every* thread of a process, not just the one that exited last. The
  process may be freed only when all its threads are off-CPU.
- [ ] **No leaked dead processes**
  Orphaned children are re-parented to init (PID 1), never to the kernel process. A
  fork/exit stress loop must bring kernel memory back to its baseline.
- [ ] **`exit_group` semantics**
  When one thread calls `exit`, every other thread in the process is killed and cleaned up.
- [ ] 🚧 **Shebang execution tested**
  `#!/bin/sh` scripts execute correctly with arguments. Covered by an automated test.

### Signals

- [x] **Correct signal frame layout**
  The frame is placed below the 128-byte red zone and written through the user virtual
  address. It records where the saved context and FPU state live.
- [x] **Safe `sigreturn`**
  The saved-context pointers are validated (`EFAULT` on garbage). Code and stack selectors are
  forced to user values, and only safe RFLAGS bits are restored, with interrupts always on.
- [x] **`sigreturn` re-queues the thread**
  A thread returning from a handler is put back on the run queue instead of being silently
  lost.
- [ ] **`sigaction` flags**
  `SA_RESTART`, `SA_RESETHAND`, `SA_NODEFER`, `SA_ONSTACK` and `SA_NOCLDWAIT` behave as POSIX
  specifies.
- [ ] **`SA_SIGINFO` handlers**
  Handlers receive a correctly filled `siginfo_t` (`si_pid`, `si_uid`, `si_code`, `si_addr`
  for faults) and a usable `ucontext_t`.
- [ ] **Signal masks across `fork` / `exec`**
  The mask is inherited on `fork` and preserved on `exec`. Handlers reset to default on
  `exec`, and ignored signals stay ignored.
- [ ] **`sigsuspend`, `sigtimedwait`, `sigpending`, `sigaltstack`**
  Real implementations, not stubs.
- [ ] **Consistent `EINTR` handling**
  A blocking syscall interrupted by a signal either returns `EINTR` or restarts (with
  `SA_RESTART`), the same way for every blocking path.
- [ ] **Fault signals**
  User page faults, illegal instructions and divide-by-zero deliver `SIGSEGV`, `SIGILL` and
  `SIGFPE` with the right `si_addr` instead of panicking the kernel.

### File descriptors

- [x] **Lowest-free fd allocation**
  `open`, `dup`, `pipe` and `socket` always return the lowest unused fd number, as POSIX
  requires.
- [x] **Per-descriptor `FD_CLOEXEC`**
  Close-on-exec belongs to the fd slot, not the shared open file. `F_GETFD`/`F_SETFD`,
  `FIOCLEX`/`FIONCLEX`, `O_CLOEXEC`, `pipe2` and `SOCK_CLOEXEC` all honour it.
- [x] **`dup2(fd, fd)`**
  Returns `fd` unchanged after validating it, instead of closing it.
- [ ] **fd syscall audit**
  Every syscall that takes an fd returns `EBADF`, `EINVAL` or `ENOTTY` on bad input, and never
  crashes or touches the wrong file.
- [ ] **`RLIMIT_NOFILE`**
  The per-process descriptor limit is enforced, and `EMFILE` is returned when it is hit.

### Kernel robustness

- [x] **Interrupt-safe spinlocks**
  `Acquire` disables interrupts *before* spinning. `Release` restores the exact saved state,
  even for nested locks.
- [x] **Interrupt-safe log sinks**
  Serial and console logging can be called from interrupt handlers without deadlocking.
- [x] **Readable exception reports**
  CPU exceptions print vector, error code, RIP, CR2 and RSP once to the debug port. A nested
  exception halts instead of recursing forever.
- [ ] **Symbolized stack traces on panic**
  Every panic prints a stack trace with demangled function names, on both architectures.
- [ ] **Debug lock checker**
  In debug builds, detect lock-order inversions, double acquisition, and sleeping or blocking
  while holding a spinlock. Document the lock-ordering rules.
- [ ] **User pointer validation everywhere**
  Every syscall copies user memory through checked helpers (`CopyFromUser`/`CopyToUser`). A
  bad pointer gives `EFAULT`, never a kernel page fault.
- [ ] **Kernel assertions in debug builds**
  Cheap invariants (list membership, refcounts, thread states) are asserted so that
  corruption is caught close to its source.

### Console / TTY

- [x] **No false EOF on reads**
  Canonical and raw TTY reads loop until data is available. A spurious wake-up never returns
  0 bytes, which a shell would treat as end of input and exit.
- [ ] **Full line-discipline control characters**
  `VINTR` (`^C`), `VQUIT` (`^\`), `VSUSP` (`^Z`), `VEOF` (`^D`), `VERASE`, `VKILL` and
  `VWERASE` work, and `ECHOCTL` echoes them as `^C`.
- [ ] **Keyboard signals go to the foreground process group**
  `^C` sends `SIGINT` to every process in the TTY's foreground process group, not just one
  process.
- [ ] **termios completeness**
  `ICANON`, `ECHO`, `ECHOE`, `ICRNL`, `ONLCR`, `OPOST`, `VMIN`/`VTIME`, and `TCSANOW` /
  `TCSADRAIN` / `TCSAFLUSH` all behave as on Linux.

### Tooling & build

- [ ] **Automated QEMU boot test in CI**
  CI boots the image headless, types commands over serial, compares the output with the
  expected results, and fails on any panic or hang.
- [ ] **Kernel unit-test harness**
  Extend `Tests/` so Prism containers, allocators and VFS helpers have tests that run in CI.
- [ ] **Correct incremental builds**
  Changing a userland program always rebuilds the initrd and image. There are no stale
  artifacts.
- [ ] **No `sudo` for development builds**
  The disk image can be rebuilt without root (for example with guestfs or mtools), so the
  edit-build-boot loop is non-interactive.

### Exit criteria for 0.1

- [ ] A shell session survives **1 hour** of scripted use: running commands, running
      pipelines, exiting the shell, and having Aurora relaunch it. No crashes or hangs.
- [ ] The CI boot test is green on every commit to `master`.
- [ ] There are no open known use-after-free or lost-wakeup bugs.

---

## 0.2: DOOM & Terminal Life

> **Theme:** the first "it's a real OS" moment, and a terminal you'd actually enjoy living in.
> DOOM is the perfect first graphical target: it needs a framebuffer, a keyboard, a clock and
> a file, and not much else.

### Headline

- [ ] 🚧 **Stage 1: DOOM draws a frame**
  `doomgeneric` (already packaged) starts, loads `doom1.wad` (the shareware WAD, or Freedoom),
  and the title screen appears on `/dev/fb0`.
- [ ] **Stage 2: DOOM is playable**
  Arrow keys, Ctrl, Space and Esc work, *including key release*. The game runs at its native
  35 tics/s, without stutter or drift.
- [ ] **Stage 3: DOOM is clean**
  The framebuffer console doesn't draw over the game. Quitting DOOM restores the text console,
  and the shell still works.
- [ ] **Stage 4: htop**
  `htop` (or `top` from procps) shows live processes and CPU usage, and quits cleanly.
- [ ] **Stage 5: job control**
  In bash: `vim`, `^Z`, `jobs`, `bg`, `fg`, `kill %1`. Everything behaves as on Linux.
- [ ] **Stage 6: tmux**
  `tmux` with a split pane: `htop` on one side, `vim` on the other. Detach, reattach, and
  resize panes. This exercises PTYs, Unix sockets, `poll`, signals and job control at once.
- [ ] **Bonus: screenshot-worthy terminal**
  `neofetch` shows the CryptixOS logo. `nyancat`, `cmatrix` and `sl` run full-screen and exit
  cleanly.

### What the demo needs

#### DOOM

- [x] **Framebuffer device and framebuffer console**
  `/dev/fb0` with `FBIOGET_VSCREENINFO` / `FBIOGET_FSCREENINFO`.
- [ ] **`mmap` of `/dev/fb0`**
  `MAP_SHARED` maps video memory straight into the process, so userland draws directly into it,
  with double buffering. `write()` + `lseek` works as a fallback.
- [ ] **evdev for the PS/2 keyboard**
  `/dev/input/event0` delivers `EV_KEY` press, release and repeat events with Linux key codes.
  It supports `O_NONBLOCK`, so a game loop can poll it. The `evtest` tool already in the
  sysroot shows the events. A normal TTY can't report key release, which is why DOOM needs
  this.
- [ ] **Accurate time**
  `clock_gettime(CLOCK_MONOTONIC)` has at least millisecond resolution. `nanosleep` / `usleep`
  sleep for about the requested time, not a whole scheduler tick extra.
- [ ] **Console graphics mode**
  `KDSETMODE KD_GRAPHICS` / `KD_TEXT` (or an equivalent) stops the fb console from drawing,
  and restores it afterwards.
- [ ] **The WAD ships in the image**
  The image build copies a freely redistributable WAD to `/usr/share/doom/`.

#### Pseudo-terminals

- [ ] 🚧 **`/dev/ptmx` + DevPtsFs**
  Opening `/dev/ptmx` creates a master/slave pair. `/dev/pts/N` can be found, opened and
  `stat`-ed, and it disappears when both sides are closed.
- [ ] **PTY ioctls**
  `TIOCGPTN`, `TIOCSPTLCK`, and mlibc's `grantpt` / `unlockpt` / `ptsname` work.
- [ ] **Controlling-terminal semantics**
  `setsid` + `TIOCSCTTY` make the PTY the session's controlling terminal. `TIOCNOTTY`
  releases it, and `tty(1)` reports the correct name.
- [ ] **Hang-up handling**
  Closing the master sends `SIGHUP` to the slave's session. Further slave reads return EOF
  and writes return `EIO`.
- [ ] 🚧 **Window size**
  `TIOCSWINSZ` / `TIOCGWINSZ` store the size per terminal and send `SIGWINCH` to the
  foreground group. `stty size` and `tput cols` agree.

#### Job control

- [ ] **Foreground process group**
  `tcsetpgrp` and `tcgetpgrp` work. Only the foreground group can read the terminal.
- [ ] **Background I/O signals**
  A background read sends `SIGTTIN`. A background write sends `SIGTTOU` when `TOSTOP` is set.
- [ ] **Stop / continue**
  `SIGTSTP`, `SIGSTOP` and `SIGCONT` stop and resume processes. `waitpid` reports this with
  `WUNTRACED` and `WCONTINUED`.
- [ ] **Shell job control**
  `^Z`, `fg`, `bg`, `jobs`, `kill %1` and `wait` work in bash and awsh.
- [ ] **Orphaned process groups**
  A stopped orphaned group gets `SIGHUP` + `SIGCONT`, as POSIX requires.

#### I/O multiplexing

- [ ] **`poll` / `select` / `pselect`**
  Work on TTYs, PTYs, pipes, FIFOs, sockets and regular files. A ready fd wakes the waiter
  immediately, and timeouts are accurate.
- [ ] **Non-blocking I/O everywhere**
  `O_NONBLOCK` returns `EAGAIN` instead of blocking, on every file type that can block.

#### Unix domain sockets (stream)

- [ ] 🚧 **Stream sockets**
  `bind`, `listen`, `accept` and `connect` (currently stubs returning `ENOSYS`), with a
  backlog and blocking or non-blocking accept. tmux needs this now, and X11 needs it in 0.4.
- [ ] **`socketpair`**

#### Userland

- [ ] **ncurses apps**
  vim, nano, less and a `top`-like tool render correctly, resize on `SIGWINCH`, and restore
  the terminal on exit.

### Floor

- [ ] **Packet mode and flow control**
  `TIOCPKT`, plus `^S` / `^Q` (`IXON`), for terminal emulators and `script(1)`.
- [x] **Anonymous pipes**
  `pipe` and `pipe2`, with blocking reads and writes.
- [ ] **Named pipes (FIFOs)**
  `mkfifo` creates a FIFO node. `open` blocks until the other end is opened, unless
  `O_NONBLOCK` is set.
- [ ] **Broken pipe**
  Writing to a pipe with no readers sends `SIGPIPE`, and returns `EPIPE` if that signal is
  ignored.
- [ ] **`PIPE_BUF` atomicity**
  Writes of up to `PIPE_BUF` bytes are never interleaved with other writers.
- [ ] **Aurora 0.2–0.3**
  Config file, a service table, restart policy and logging (see its own roadmap).
- [ ] **awsh as a daily shell**
  Line editing, history, job control and scripting good enough to replace bash
  interactively.

### Exit criteria for 0.2

- [ ] You play DOOM E1M1 from start to exit, quit, and land back in a working shell.
- [ ] The tmux demo works, including resizing, detaching and reattaching.
- [ ] `vim` inside a PTY session resizes correctly. `^C` and `^Z` behave as on Linux.
- [ ] `cat big | grep x | sort | uniq -c | head` gives correct output, and all processes exit
      cleanly.
- [ ] The signal, pipe and process-group parts of the Open POSIX Test Suite pass.

---

## 0.3: Online, and nmap

> **Theme:** CryptixOS talks to the world, and can *look* at the world. Each stage is a
> separate win, from the first ping reply to a full nmap SYN scan.

### Headline

- [ ] **Stage 1: first packet**
  `ping 10.0.2.2` (the QEMU host) gets replies. `ping` from the host to CryptixOS works too.
- [ ] **Stage 2: talk to the host**
  `nc` on CryptixOS chats with `nc` on the host, in both directions. Your own `portscanner`
  (already packaged) finds open ports on the host.
- [ ] **Stage 3: the web**
  `curl https://example.com` downloads a page over TLS (openssl is already in the sysroot),
  with the hostname resolved via DNS.
- [ ] **Stage 4: nmap connect scan**
  `nmap -sT -p- <host>` and `nmap -sT scanme.nmap.org` run to completion with correct results.
  `nmap -sV` identifies service versions.
- [ ] **Stage 5: nmap, the real thing**
  `nmap -sS -O` (SYN scan and OS detection) works as root on a TAP/bridged network. This needs
  raw sockets and packet capture, the deepest test of the stack.

> Only scan machines you own, plus `scanme.nmap.org`, which the nmap project explicitly allows.
> QEMU's default user-mode network (slirp) handles stages 1–4. Stage 5 needs `-netdev tap` or
> a bridge, because slirp doesn't pass raw packets through.

### What the demo needs

#### Network stack

- [ ] **Loopback interface**
  `lo` with `127.0.0.1`, so local TCP and UDP work without a NIC. A good first target: it tests
  the whole TCP state machine without any driver.
- [ ] **Ethernet + ARP**
  Frame parsing, an ARP cache with timeouts, and gratuitous ARP.
- [ ] **IPv4 + ICMP**
  Routing table, fragmentation and reassembly, ICMP echo (so `ping` works), and ICMP errors
  (port unreachable, TTL exceeded) passed up to sockets.
- [ ] **UDP**
  `sendto` / `recvfrom`, broadcast, and port binding.
- [ ] **TCP**
  A full state machine, retransmission with RTT estimation, sliding windows, `SO_REUSEADDR`,
  keep-alive, and clean `FIN`/`RST` handling.
- [ ] **Non-blocking `connect`**
  `connect` on a non-blocking socket returns `EINPROGRESS`. `poll` reports completion, and
  `getsockopt(SO_ERROR)` gives the result. This is how `nmap -sT` scans thousands of ports in
  parallel.
- [ ] **Interface configuration**
  `SIOC*` ioctls and/or NetLink `RTM_*`, so `ip` and `ifconfig` can set addresses and routes.
  nmap's bundled libdnet needs `SIOCGIFCONF` and `SIOCGIF{FLAGS,ADDR,NETMASK,HWADDR,MTU,INDEX}`,
  plus `/proc/net/route` and `/proc/net/dev`.
- [ ] **DHCP client**
  A userland daemon started by Aurora.
- [ ] **DNS**
  `getaddrinfo` in mlibc using `/etc/resolv.conf` and `/etc/hosts`.

#### Raw packets (stage 5)

- [ ] **`SOCK_RAW` for `AF_INET`**
  `IPPROTO_ICMP`, `IPPROTO_TCP`, `IPPROTO_UDP` and `IP_HDRINCL`, root only.
- [ ] **`AF_PACKET` sockets**
  `SOCK_RAW` / `SOCK_DGRAM`, `bind` to an interface, and `recvfrom` with `sockaddr_ll`.
  libpcap's Linux backend runs on this. A `TPACKET` ring and `SO_ATTACH_FILTER` (BPF) are
  optional, because libpcap can filter in userspace.
- [ ] **Promiscuous mode**
  `PACKET_ADD_MEMBERSHIP` / `IFF_PROMISC` reach the NIC driver.

#### Drivers

- [x] **RTL8139** (basic)
- [ ] **Intel e1000 / e1000e**
  Very common in emulators and on real hardware.
- [ ] **virtio-net**
  The default fast NIC for QEMU/KVM.

#### Ports

- [ ] **Network tools ported**
  `ping`, `nc`, `ip`/`ifconfig`, `curl` or `wget`, `tcpdump` (a great debugging tool for the
  stack itself), and `nmap`. nmap needs libstdc++, libpcap and pcre2. Build it with
  `--without-zenmap` and, at first, `--without-liblua`.
- [ ] **SSH client**
- [ ] **A small HTTP server**

### Floor

- [ ] **Datagram and seqpacket Unix sockets**
  `SOCK_DGRAM` and `SOCK_SEQPACKET` keep message boundaries.
- [ ] **fd and credential passing**
  `SCM_RIGHTS` and `SCM_CREDENTIALS` via `sendmsg` / `recvmsg`, plus `SO_PEERCRED`.
- [ ] **Abstract namespace and `getsockopt`/`setsockopt` basics**
- [ ] **`epoll`**
  `epoll_create1`, `epoll_ctl` and `epoll_wait` with level-triggered semantics.
  Edge-triggered mode is a stretch goal.
- [ ] **IPv6** (optional for 1.0)

### Exit criteria for 0.3

- [ ] Boot in QEMU, get an address over DHCP, resolve a hostname, and download a file over
      HTTPS with `curl`.
- [ ] `nmap -sT` against the host matches what `nmap` on the host itself reports.
- [ ] A 1 GiB TCP transfer completes with the correct checksum.

---

## 0.4: X11

> **Theme:** windows, a mouse pointer, and DOOM in a window. This is the jump from "hobby
> kernel" to "that's an operating system".

### Headline

- [ ] **Stage 1: X starts**
  `Xorg` with the `fbdev` driver starts, shows the classic grey root weave and the X cursor, and
  quits cleanly on Ctrl+Alt+Backspace.
- [ ] **Stage 2: xeyes**
  `xeyes` follows the mouse. This proves the client connection, input and drawing all work.
- [ ] **Stage 3: a desktop**
  `twm` (or another small window manager) with `xterm` and `xclock`. Windows can be moved and
  resized, and `xterm` runs bash, vim and htop.
- [ ] **Stage 4: DOOM in a window**
  DOOM runs inside X (doomgeneric's X11 backend, or an SDL2 source port such as Chocolate
  Doom).
- [ ] **Stage 5 (stretch): graphical web**
  A small graphical browser (for example NetSurf) loads a web page, using the 0.3 network
  stack.

### What the demo needs

#### Kernel

- [ ] **PS/2 mouse**
- [ ] **evdev for every input device**
  Every input device appears as `/dev/input/eventN` with Linux-compatible events (`EV_KEY`,
  `EV_REL`, ...), plus the `EVIOCG*` ioctls libevdev needs (`EVIOCGBIT`, `EVIOCGNAME`,
  `EVIOCGID`, `EVIOCGRAB`, ...). `evtest` works for all of them.
- [ ] **Correct `MAP_SHARED` of the framebuffer**
  With correct `smem_len`/`line_length`, and caching attributes set for performance.
- [ ] **File-backed `mmap` good enough for `dlopen`**
  Xorg loads its video and input drivers as `.so` modules. `MAP_PRIVATE` file mappings with
  correct protections (read on demand, or copy at map time as a first step) are enough. This
  pulls part of 0.5 forward, intentionally.
- [ ] **VT ioctls**
  `VT_GETSTATE`, `VT_OPENQRY`, `VT_ACTIVATE`, `VT_WAITACTIVE`, `VT_GETMODE`/`VT_SETMODE`,
  `KDGKBMODE`/`KDSKBMODE`, and `KDSETMODE`. Starting Xorg with `-keeptty -novtswitch`
  postpones most of these.
- [ ] **pthreads that hold up**
  Futex wait/wake with no lost wake-ups, for Xorg's input thread. Building with
  `--disable-input-thread` is the fallback.
- [ ] **Shared memory** (optional here, required in 0.5)
  `shm_open` / `memfd_create` or SysV `shmget`, for MIT-SHM. X works without it, just slower.

#### Ports

- [ ] **X libraries**
  xorgproto, libXau, libXdmcp, xcb-proto, libxcb, libX11, libXext, libXrender, libXfont2,
  libxkbfile, pixman, libevdev, mtdev, freetype, harfbuzz, fontconfig and xkeyboard-config.
  (Managarm's and Astral's xbstrap recipes are a good reference.)
- [ ] **X server and drivers**
  xorg-server, xf86-video-fbdev and xf86-input-evdev, plus a minimal `xorg.conf`.
- [ ] **Clients**
  xinit, twm, xterm, xeyes, xclock, xsetroot and some fonts.

### Floor

- [ ] **Guard pages**
  User stacks grow down with a guard page. Kernel stacks have guard pages, so an overflow
  faults instead of corrupting a neighbour.
- [ ] **`munmap` / `mprotect` on partial ranges**
  Regions split correctly. Big programs like Xorg do this constantly.
- [ ] **Terminal emulator and fonts**
  quantum-term running on a font stack (freetype and harfbuzz), plus a few GUI apps.

### Exit criteria for 0.4

- [ ] A graphical session with twm, two xterms and xeyes runs for **1 hour** without
      problems.
- [ ] DOOM runs in an X window at full speed.
- [ ] Quitting X returns to a working text console.

---

## 0.5: Real Memory Management

> **Theme:** X and nmap now show you exactly where the VM is slow or leaky. Make it efficient
> and correct, Unix-style.

### Headline

- [ ] **Stage 1: Python**
  `python3` (already in the sysroot) runs scripts that use threads, `subprocess` and `mmap`.
- [ ] **Stage 2: the first native compile**
  `gcc hello.c -o hello && ./hello` works *on* CryptixOS. gcc forks `cc1`, `as` and `ld`, so
  this stresses everything at once.
- [ ] **Stage 3: benchmark numbers**
  `fork` of a process using 1 GiB of memory takes milliseconds. `htop` and `/proc` show real
  memory numbers. Post the before and after benchmark.

### What the demo needs

- [x] **Lazy (demand) page allocation**
  Anonymous memory is mapped on first touch, not up front.
- [ ] **Copy-on-write `fork`**
  The child shares the parent's pages read-only. A write fault copies just that page, so
  `fork` of a large process becomes cheap.
- [ ] 🚧 **File-backed `mmap`, `MAP_PRIVATE`** (complete)
  File pages are read on demand through the page cache and copied on write. This is how shared
  libraries and executables get loaded.
- [ ] **File-backed `mmap`, `MAP_SHARED`**
  Writes go to the shared page and reach the file via the page cache. `msync` flushes them.
- [ ] **`mremap` and `madvise`** (`DONTNEED`, `WILLNEED`)
- [ ] **Shared memory**
  `memfd_create` and `shm_open` (on tmpfs). System V `shmget` if ports need it.
- [ ] **Robust futexes**
  `FUTEX_WAIT`, `WAKE`, `REQUEUE`, `WAIT_BITSET`, and private futexes, with no lost wake-ups.
  pthread mutexes, condvars and `pthread_join` built on them pass tests.
- [ ] **Unified page cache**
  `read`/`write` and `mmap` share the same cached pages, and clean pages can be reclaimed.

### Floor

#### Allocators

- [x] **Physical bitmap allocator**
- [x] **Slab heap allocator**
- [x] **Virtual range allocator (red-black tree)**
- [ ] **Buddy physical allocator**
  Gives fast allocation of contiguous, power-of-two blocks, needed for DMA buffers and large
  pages.
- [ ] **Per-CPU slab caches**
  Lets most allocations skip the global lock.
- [ ] **NUMA-aware allocation**
  Prefer pages from the node local to the CPU that asked for them.
- [ ] **Huge pages** (optional)
  Map 2 MiB pages for the kernel direct map and for large anonymous mappings.

#### Pressure & limits

- [ ] **Per-process memory accounting**
  RSS and virtual size are shown in `/proc/<pid>/status`. `getrusage` reports real numbers.
- [ ] **Memory limits enforced**
  `RLIMIT_AS`, `RLIMIT_DATA` and `RLIMIT_STACK` make allocations fail with `ENOMEM` instead of
  exhausting the system.
- [ ] **OOM handling**
  When memory runs out, the kernel kills the largest offending process and logs why, instead
  of panicking.
- [ ] **Swap** (optional for 1.0)
  Anonymous pages can be paged out to a swap partition or file.

#### Other

- [ ] **`eventfd` / `timerfd` / `signalfd`**
  Needed by many modern ports.

### Exit criteria for 0.5

- [ ] gcc compiles and runs a multi-file C program on CryptixOS.
- [ ] Forking a process using 1 GiB of memory takes milliseconds. `/proc` memory numbers
      match reality.
- [ ] Dynamically linked programs load `.so` files through cached, `mmap`-ed pages.
- [ ] A 1-hour loop of fork + mmap + munmap leaks no memory and causes no panics.

---

## 0.6: Your Data Survives

> **Theme:** your data is safe.

### Headline

- [ ] **Stage 1: git**
  `git clone https://…` a real repository onto ext2.
- [ ] **Stage 2: build it there**
  Build the cloned project on CryptixOS, using the 0.5 compiler.
- [ ] **Stage 3: reboot**
  Reboot. Everything is still there, and `e2fsck -f` reports a clean filesystem.

### What the demo needs

#### VFS semantics

- [ ] **Atomic `rename`**
  Within and across directories, including replacing an existing target, with no window
  where neither name exists.
- [ ] **Links**
  `link`, `symlink` and `readlink` work, symlink loops give `ELOOP`, and unlinking an open
  file keeps its data until the last close.
- [ ] **Complete `*at` family**
  `openat`, `fstatat`, `statx`, `utimensat`, `fchmodat`, `fchownat`, `unlinkat` and
  `renameat2`, with `AT_*` flags.
- [ ] **File locking**
  `flock`, plus `fcntl` `F_SETLK` / `F_SETLKW` / `F_GETLK` with correct ownership and release
  on close.
- [ ] **`inotify`**
  Basic create, modify, delete and move events for watched files and directories.
- [ ] 🚧 **Mount features**
  Bind mounts, read-only mounts (`MS_RDONLY`), remount, `pivot_root`, and `umount` refusing
  busy mounts.

#### Block layer & drivers

- [ ] **Buffer cache with write-back**
  Dirty blocks are flushed periodically and on `sync`. `fsync` / `fdatasync` wait for the
  data to reach the disk.
- [ ] **Flush on shutdown**
  Reboot and poweroff unmount filesystems and flush every cache first.
- [x] **MBR partitions**
- [x] **GPT partitions**
- [ ] **EBR (logical partitions)**
- [x] **NVMe**
- [ ] **AHCI / SATA**
  Needed for most real machines and for QEMU's default `-hda`.
- [ ] **virtio-blk**
  A fast paravirtual disk for QEMU/KVM.
- [ ] **ATAPI** (optional)
  CD-ROM drives.

### Floor

#### Filesystems

- [x] **ext2**: read and write.
- [x] **FAT32**
- [x] **tmpfs**
- [x] **procfs** (basic)
- [x] **devtmpfs**
- [x] **initrd**
- [ ] **fsck-clean ext2**
  After heavy use (package builds, many small files, deletes), `e2fsck -f` reports no errors.
- [ ] **ext4 read support** (optional)
  Extents and 64-bit features, so images made by Linux can be mounted read-only.
- [ ] **SysFs**
  `/sys/class`, `/sys/block`, `/sys/bus/pci/devices`, enough for `lsblk`, `lspci` and
  udev-like tools.
- [ ] **procfs completeness**
  `/proc/<pid>/{stat,status,maps,fd,cmdline,environ,cwd,exe}`, plus `/proc/meminfo`,
  `/proc/cpuinfo`, `/proc/mounts` and `/proc/uptime`, in Linux-compatible formats so that
  procps works.
- [ ] **ISO9660**
  Mount the boot CD and install media.
- [ ] **Loop devices**
  Mount a disk image file as a block device.
- [ ] **EfiVarsFs**
  Read and write UEFI variables (needed by an installer for boot entries).

### Exit criteria for 0.6

- [ ] Unpack and build a package on ext2, reboot, then `e2fsck -f` reports a clean
      filesystem.
- [ ] A clean shutdown never loses written data. A crash loses at most the data written since
      the last sync.

---

## 0.7: Real Hardware

> **Theme:** runs on real machines, not just in QEMU.

### Headline

- [ ] **Stage 1: it boots**
  CryptixOS boots from a USB stick on a real machine to a shell.
- [ ] **Stage 2: it's usable**
  A USB keyboard and mouse work, and X runs on the real screen at its native resolution.
- [ ] **Stage 3: DOOM with sound**
  DOOM with sound effects and music, on real hardware.
- [ ] **Stage 4: nmap from real hardware**
  CryptixOS gets a DHCP lease on your home LAN with a real NIC and `nmap`s your router.

### What the demo needs

#### USB

- [ ] 🚧 **UHCI**
- [ ] **xHCI**
  The most important controller on modern hardware. EHCI/OHCI are optional.
- [ ] **Hubs, enumeration and hot-plug**
  Devices behind hubs are found, and plugging or unplugging is handled at runtime.
- [ ] **USB HID**
  Keyboards and mice, reported through evdev.
- [ ] **USB mass storage**
  Bulk-only transport and SCSI commands, so USB sticks appear as block devices.

#### Graphics on real hardware

- [ ] **Mode setting**
  Change resolution where the firmware or GPU allows it. Expose EDID information.
- [ ] **Display API**
  A small DRM/KMS-like interface (dumb buffers, page flip) that compositors can target.

#### Sound

- [ ] **Intel HDA** (and AC'97 for QEMU, see [side quests](#side-quests))
- [ ] **OSS-style `/dev/dsp`**, or a small ALSA-compatible API
  Enough for SDL2's audio backend and DOOM's sound.

#### Platform

- [x] **ACPI (uACPI), PCI/PCIe, HPET, LAPIC, I/O APIC, RTC, KVM clock**
- [x] **Reboot and shutdown**
- [x] **PS/2 keyboard** (i8042)
- [ ] **MSI / MSI-X for all PCI drivers**
  Avoids shared legacy interrupt lines and scales across CPUs.
- [ ] **ACPI events**
  Power button, lid, battery status and thermal zones, through the embedded controller.
- [ ] **I²C**
  Needed for touchpads and sensors on many laptops.

### Floor

- [ ] **Device uevents**
  Hot-plug events reach userland over NetLink, so a udev-like daemon can create `/dev` nodes.
- [ ] **Keymaps**
  Loadable keyboard layouts for the VT console (`loadkeys`-like).
- [ ] **Sleep (S3)** (optional for 1.0)

### Exit criteria for 0.7

- [ ] CryptixOS boots to a usable shell, and then to X, on **at least two real x86_64
      machines** (a desktop and a laptop), using a USB keyboard and an NVMe or SATA disk.

---

## 0.8: Multi-User & Hardened

> **Theme:** a real multi-user system that a normal user cannot break.

### Headline

- [ ] **Stage 1: login**
  Aurora starts `getty` on each VT. You log in as a normal user with a password.
- [ ] **Stage 2: ssh in**
  `ssh user@cryptix` from your main machine works (dropbear or OpenSSH), and `sudo` works
  inside.
- [ ] **Stage 3: try to break it**
  That user can't read others' files, modify `/etc`, or panic the kernel. 24 hours of syscall
  fuzzing finds nothing.

### What the demo needs

#### Identity & login

- [ ] **Credentials model**
  Real, effective and saved UID/GID plus supplementary groups, inherited on `fork` and checked
  everywhere permissions matter.
- [ ] **Permission enforcement**
  `rwx` bits, `umask`, sticky directories and search permission are checked during path
  resolution, `open`, `exec` and `unlink`.
- [ ] **setuid / setgid executables**
  `exec` of a setuid binary changes the effective ID. It is ignored on `nosuid` mounts.
- [ ] **Account files**
  `/etc/passwd`, `/etc/shadow` and `/etc/group`, used by mlibc's `getpwnam` and friends.
- [ ] **Login flow**
  Aurora starts a `getty` on each VT. `login` authenticates through Linux-PAM and starts the
  user's shell in their home directory.
- [ ] **Privilege escalation**
  `su`, plus a `sudo`/`doas` port.
- [ ] **Capabilities**
  At least `CAP_SYS_ADMIN`, `CAP_DAC_OVERRIDE`, `CAP_NET_ADMIN`, `CAP_NET_RAW` (raw sockets for
  nmap), `CAP_NET_BIND_SERVICE` and `CAP_KILL`, checked instead of plain "uid == 0".
- [ ] **SSH server**
  dropbear or OpenSSH, started by Aurora.

### Floor

#### Hardening

- [ ] **NX and W^X**
  No page is both writable and executable, in kernel or user space.
- [ ] **SMEP / SMAP (x86_64), PAN / PXN (aarch64)**
  The kernel can neither execute nor accidentally read user memory.
- [ ] **User-space ASLR**
  Stack, `mmap` base, heap and PIE executables are randomized per process.
- [ ] **Kernel ASLR** (optional)
- [ ] **Stack protector**
  The kernel is built with `-fstack-protector-strong`, and a canary failure panics loudly.
- [ ] **Resource limits**
  `RLIMIT_NPROC` stops fork bombs. `RLIMIT_NOFILE` and `RLIMIT_CPU` are enforced.
- [ ] **`chroot`**
- [ ] **Syscall fuzzing in CI**
  A random-syscall fuzzer (syzkaller-style) runs against every release candidate.

### Exit criteria for 0.8

- [ ] An unprivileged user cannot read other users' files, modify `/etc`, or crash the kernel
      through any known syscall.
- [ ] The fuzzer runs for 24 hours with zero kernel panics.

---

## 0.9: Self-Hosting, aarch64 & Beta

> **Theme:** the OS builds itself, runs on a second architecture, and then gets polished.
> Feature freeze starts once the self-hosting demo passes.

### Headline

- [ ] **Stage 1: CryptixOS builds CryptixOS**
  Clone the repo on CryptixOS, build the kernel and core userland, and boot the result.
- [ ] **Stage 2: DOOM on aarch64**
  The same image recipe produces an aarch64 build that runs bash and DOOM on QEMU `virt`.
- [ ] **Stage 3 (optional): a real board**
  For example a Raspberry Pi 4/5.

### What the demo needs

#### Self-hosting

- [ ] **Native toolchain**
  GCC or Clang, binutils, make, meson, ninja, python and git running *on* CryptixOS.
- [ ] **Rebuild itself**
  CryptixOS builds its own kernel and core userland from source, and the result boots.
- [ ] **Package management**
  Install, upgrade and remove xbstrap-built packages from inside the running system.

#### aarch64 parity

- [ ] 🚧 **Core bring-up**
  Boot, GIC, exceptions and VMM work.
- [ ] **Timers and SMP**
  The generic timer as the scheduler tick. PSCI to bring up secondary cores.
- [ ] **Devices**
  The PL011 serial driver with interrupt-safe locking, plus virtio-blk/net/input on QEMU
  `virt`.
- [ ] **Signals and userland**
  The signal trampoline and frame, and mlibc + bash + coreutils working on aarch64.
- [ ] **aarch64 CI boot test**
  The same headless test as x86_64, green on every commit.

### Floor

#### Performance

- [ ] **Scheduler policies**
  Priorities and `nice`, `sched_setaffinity`, load balancing across cores, and an idle CPU
  that actually sleeps.
- [ ] **Lock contention**
  Fine-grained locking in the hottest paths (VFS lookup, page faults, run queues), guided by
  profiling.
- [ ] **Profiling tools**
  A kernel sampling profiler, accurate `/proc/<pid>/stat` CPU times, and correct `time(1)`.
- [ ] **Benchmarks tracked over time**
  Boot time, fork/exec latency, pipe and socket throughput, disk read/write speed, and DOOM
  timedemo FPS, recorded in CI so regressions are visible.

#### Quality

- [ ] **Feature freeze**
  After the self-hosting demo passes, only bug fixes, tests and documentation are merged.
- [ ] **POSIX conformance report**
  The Open POSIX Test Suite pass rate is published, with every known failure listed and
  explained.
- [ ] **No reachable panics**
  No known kernel panic can be triggered from userland.
- [ ] **Documentation**
  The build guide, a one-page overview for each subsystem, a syscall list with conformance
  notes, and a driver-writing guide.

### Exit criteria for 0.9

- [ ] A kernel built on CryptixOS boots successfully.
- [ ] Release candidates ship (`1.0-rc1`, `rc2`, ...), and the last one goes an agreed period
      with no new critical bugs.

---

## 1.0: First Stable Release

> **Theme:** a small but complete, stable, self-hosting Unix-like OS with a graphical session.
>
> 1.0 means: *you can install it, use it daily in a VM or on supported hardware, and it will
> not lose your data or crash under normal use.*

- [ ] **Installer**
  Partitions a disk, creates filesystems, installs the base system, and installs and
  configures Limine (including UEFI boot entries).
- [ ] **Release images**
  A live ISO and a disk image for x86_64 (and aarch64 if parity is reached), with SHA-256
  checksums.
- [ ] **Release notes**
  Features, supported hardware, known issues and upgrade notes.
- [ ] **Syscall ABI policy**
  Says which syscalls and structures are guaranteed stable for all 1.x releases.
- [ ] **Versioning policy**
  `1.x.y` patch releases are bug fixes only. `1.x` minor releases add features without
  breaking the ABI.
- [ ] **Security process**
  Explains how to report vulnerabilities and how fixes are released.

---

## Side quests

Small, self-contained wins for when you need a quick bit of progress between bigger stages.
Each one lists what it depends on. None of them blocks a version, so pick any one that is
unlocked.

| Side quest | Unlocked after | What it proves |
|---|---|---|
| Boot splash logo on the framebuffer | now | fb drawing from the kernel |
| `cowsay`, `figlet`, `fortune`, `lolcat` | now | trivial ports, instant screenshots |
| `lua`, `tcc` (compile C *on* the OS before gcc works) | now | interpreters and small compilers |
| `nethack`, `frotz` (Zork), `2048`, `tetris` in the terminal | 0.1 | ncurses + raw TTY |
| A tiny native fb demo (plasma, starfield, Mandelbrot) | 0.2 | your own graphics code |
| AC'97 driver in QEMU, plus DOOM sound effects | 0.2 | first audio |
| `lynx` / `w3m` / `links` browsing the web in a terminal | 0.3 | TCP + DNS |
| `irssi` on Libera.Chat: say hi from CryptixOS | 0.3 | long-lived TCP + TLS |
| `ssh` *from* CryptixOS into your host | 0.3 | crypto + PTYs + network |
| A tiny HTTP server serving a page to your host's browser | 0.3 | listening TCP sockets |
| `wget` the DOOM WAD from the internet instead of baking it in | 0.3 | a full network round trip |
| Quake (SDL2 or fb port) | 0.4 | heavier graphics + FPU |
| `mpv`/`ffplay` playing a video | 0.5 | `mmap`, threads, audio |
| `neofetch` from a real laptop | 0.7 | the ultimate screenshot |

---

## Hall of fame

Record every milestone the day it happens: date, commit, and a screenshot or GIF in
`Documentation/milestones/`.

| Milestone | Date | Commit | Proof |
|---|---|---|---|
| First boot to a shell | | | |
| Shell survives 1 hour of scripted use | | | |
| DOOM title screen | | | |
| DOOM E1M1 completed | | | |
| tmux split panes | | | |
| First `ping` reply | | | |
| `curl https://…` | | | |
| First `nmap` scan | | | |
| X root weave | | | |
| xeyes follows the mouse | | | |
| DOOM in an X window | | | |
| `gcc hello.c` on CryptixOS | | | |
| Boots on real hardware | | | |
| `ssh` into CryptixOS | | | |
| CryptixOS builds CryptixOS | | | |

---

## Must-Have Before 1.0

If the version plan above slips or is reordered, these remain **non-negotiable** for 1.0:

- [ ] **Stability**
  No known panics, deadlocks or memory corruption under normal use. A 24-hour stress run
  passes.
- [ ] **Processes & signals**
  POSIX-correct fork/exec/wait/exit, full `sigaction` (flags and `siginfo`), job control,
  process groups and sessions.
- [ ] **Memory**
  Copy-on-write fork, file-backed `mmap` with a page cache, futexes and an OOM policy. No
  kernel memory leaks when processes exit.
- [ ] **Filesystems**
  Safe ext2 writes (fsck-clean), working sync/fsync, enforced permissions, and procfs, sysfs
  and devpts complete enough for standard tools.
- [ ] **Storage**
  NVMe, AHCI and virtio-blk drivers.
- [ ] **Terminals**
  VT console and PTYs with full termios, window size and job control. ncurses apps and tmux
  work.
- [ ] **I/O**
  `poll` / `select` / `epoll` and non-blocking I/O on every fd type.
- [ ] **Graphics**
  `mmap`-able framebuffer, mode setting, a DRM/KMS-like display API, an X11 session with a
  window manager and terminal emulator, and DOOM.
- [ ] **Users**
  Multi-user login, uid/gid permission checks and setuid.
- [ ] **Security**
  NX, SMEP/SMAP, user ASLR and validated user pointers. The syscall fuzzer finds no panics.
- [ ] **Networking**
  Complete Unix sockets. IPv4, TCP, UDP, ICMP, DHCP and DNS. Raw and packet sockets (nmap
  works). virtio-net and e1000 drivers.
- [ ] **Hardware**
  Boots on real x86_64 hardware, with xHCI USB keyboard support and evdev input.
- [ ] **Userland**
  Aurora with service supervision, bash and awsh, coreutils, an editor, and networking tools.
- [ ] **Self-hosting**
  Builds its own kernel and core userland.
- [ ] **Tooling**
  CI builds, boots and runs tests (including the milestone demos) on every commit. Development
  builds need no `sudo`.
- [ ] **Docs**
  Build, install and architecture documentation, plus the syscall conformance list.

### Nice to have (allowed to slip past 1.0)

- [ ] Swap
- [ ] IPv6
- [ ] Sleep / hibernate (S3/S4)
- [ ] Bluetooth
- [ ] A full graphical desktop beyond a basic compositor
- [ ] ext4 write support
- [ ] NUMA optimisations and huge pages
- [ ] Real aarch64 hardware support
- [ ] Plan9Fs
- [ ] Kernel ASLR
- [ ] Containers / namespaces

---
