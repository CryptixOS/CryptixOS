# 🔹 Init System Roadmap (0.1 → 1.0)

## 0.1 – Skeleton / Minimal Boot

- PID 1 process that runs as the first userland program.
- Prints "Init starting..." to the console.
- Forks a simple shell (awsh) to allow user commands.
- Waits for the shell to exit (waitpid).
- Handles kernel shutdown/reboot signals (simple signal handling).

## 0.2 – Static Service Startup

- Read a static config file (/etc/init.conf) listing services.
- Fork and exec services sequentially.
- Basic logging to console (stdout / /dev/kmsg).
- Handle child termination with waitpid to avoid zombies.

## 0.3 – Service Monitoring

- Keep a table of running services (PID, command, status).
- Detect service crashes and optionally restart them.
- Implement basic dependency handling: start core services first.
- Add simple logging for service failures.

## 0.4 – Signal Handling & Shutdown

- Handle SIGTERM, SIGINT, and SIGPWR signals.
- Implement graceful shutdown: send signals to all child processes, wait for termination.
- Reboot/poweroff hooks via kernel syscalls.
- Add console-based menu for shutdown/reboot (optional).

## 0.5 – Parallel Startup

- Implement parallel service startup to speed boot.
- Track dependencies: e.g., networking starts after network device driver.
- Add timeout handling for services that hang on startup.
- Logging improvements (timestamped logs, optional log files).

## 0.6 – Configuration & Extensibility

- Support service configuration files (/etc/init.d/<service> or similar).
- Parse service metadata: name, command, dependencies, autostart.
- Implement enable/disable flags per service.
- Support environment variables for services.
- Add command-line options for init (single-user mode, debug mode).

## 0.7 – Runtime Management / Monitoring

- Implement initctl tool for runtime management:
  - initctl start <service>
  - initctl stop <service>
  - initctl status <service>
- Add service restart policy (always, on-failure, never).
- Optional: simple console UI for viewing running services.

## 0.8 – Advanced Features

- Service dependency graph: topological sort before startup.
- Support service groups (networking, logging, daemons).
- Ability to reload configuration at runtime.
- Logging to persistent storage (/var/log/init.log).

## 0.9 – Security & Stability

- Enforce user/group for services (UID/GID).
- Capabilities for critical services (like network or storage).
- Harden PID 1 against crashes (cannot be killed).
- Add integration with systemd-like sockets (optional for IPC).
- Stress-test service supervision under high-load scenarios.

## 1.0 – Stable Release

- Fully functional PID 1 process.
- Parallel startup with dependencies resolved.
- Full service management tools (initctl + optional GUI console).
- Graceful shutdown/reboot.
- Robust logging & monitoring.
- Configurable, extendable, secure.
- Ready for general CryptixOS release (1.0).Y4:0
