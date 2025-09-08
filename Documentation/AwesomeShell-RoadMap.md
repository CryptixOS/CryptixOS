# AwesomeShell Roadmap (POSIX-compliant shell)

### 0.1 – Minimal Interactive Shell
- Accept input from console (`stdin`) line-by-line.
- Execute single external commands via `fork()` + `execve()`.
- Handle basic arguments (e.g., `ls -l /etc`).
- Implement `exit` command.
- Print a simple prompt (e.g., `awsh>`).

### 0.2 – Built-ins & Environment
- Implement core built-ins: `cd`, `pwd`, `echo`.
- Read and store environment variables (e.g., `PATH`, `HOME`).
- Search for executables in `PATH`.
- Implement `export` command to set environment variables.
- Handle basic command-line parsing (split words, handle quotes).

### 0.3 – Redirection & Pipes
- Output redirection: `>` and `>>`.
- Input redirection: `<`.
- Simple pipes: `ls | grep foo`.
- Support multiple chained pipes: `cat file | grep foo | sort`.
- Handle errors gracefully (file not found, permission denied).

### 0.4 – Job Control (Foreground/Background)
- Run commands in background with `&`.
- Keep track of process table for background jobs.
- Implement `jobs` command to list running background jobs.
- Implement `fg` and `bg` commands to move jobs between foreground and background.
- Handle `Ctrl-C` and `Ctrl-Z` signals for job control.

### 0.5 – Command Substitution & Simple Scripting
- Command substitution: `` `command` `` and `$(command)`.
- Support simple shell scripts: `#!/bin/awsh` as interpreter.
- Implement `if`, `while`, `for` loops (basic POSIX-compliant syntax).
- Implement variable expansion: `$VAR`.
- Implement simple comments (`# ...`).

### 0.6 – Advanced Redirection & Here-documents
- Support append redirection (`>>`) fully.
- Implement here-documents (`<<EOF`).
- Handle file descriptor management (fd 0/1/2).
- Implement input/output error handling with proper exit codes.

### 0.7 – Functions, Aliases, and Built-ins Expansion
- Support user-defined functions: `myfunc() { ... }`.
- Implement aliases: `alias ll='ls -l'`.
- Add more built-ins: `export`, `unset`, `source`, `type`.
- Implement command hashing / caching for faster lookup.

### 0.8 – Job Control Enhancements & Signals
- Implement full POSIX signal handling for child processes.
- Support `SIGCHLD`, proper reaping of zombie processes.
- Advanced job management: `disown`, `kill %job`.
- Implement `trap` command: `trap 'echo bye' EXIT`.
- Support terminal control: job suspension and resumption.

### 0.9 – Scripting Completeness
- Full POSIX-compatible scripting constructs: `case`, `select`, `until`.
- Implement shell built-in arithmetic: `$((...))`.
- Support arrays (basic form).
- Implement shell expansion: brace expansion `{1..5}`, pathname expansion `*`, `?`.
- Implement exit codes properly for pipelines and scripts.
- Unit testing framework for scripts and shell built-ins.

### 1.0 – Stable, Fully POSIX-Compliant Shell
- Complete command parsing (quotes, escapes, nested commands).
- Job control fully integrated with init system and process table.
- Robust signal handling, reaping, and error reporting.
- Script execution fully stable, supports shebang `#!/bin/awsh`.
- Built-in functions and aliases fully functional.
- Environment variable management fully POSIX-compliant.
- User-friendly prompt with support for custom formatting (`PS1`).
- Fully testable and stable enough for **CryptixOS 1.0** release.

---

## Notes:
- Start extremely minimal (0.1) and gradually layer features.
- Always test each stage by writing small shell scripts or commands.
- Use **BusyBox** or **sash** as reference implementations, but keep your code modular.

