# Supported Flags

## Kernel Configuration Flags

### ACPI Settings
- `acpi.enable`: Enable/disable ACPI support (`true` | `false`)
- `acpi.rsdt`: Prefer RSDT over XSDT (`true` | `false`)

### Logging Options
- `log.level`: Set log level (`debug`, `trace`, `info`, `warn`, `error`, `fatal`)
- `log.e9`: Enable logging through E9 port (`true` | `false`)
- `log.serial`: Enable logging through serial port (`true` | `false`)
- `log.boot.terminal`: Print boot logs to the terminal (`true` | `false`)

### Scheduling and Interrupt Handling
- `x86.sched_timer`: Choose scheduler timer (`pit` | `lapic`)
- `x86.irq_ctrl`: Choose IRQ controller (`pic` | `ioapic`)

### PCI Access
- `pci.access`: Choose PCI access mechanism (`ecam` | `legacy` (x86_64 only))

### CPU and AP Configuration
- `cpu.ap_count`: Specify number of APs to boot (`<number>`)

### Memory Mapping
- `mm.memmap`: Specify memory map parser for pmm (`limine` | `efi`)

### Panic Action
- `panic.action`: Specify action on kernel panic (`hcf` | `shutdown` | `reboot`)

### Initialization
- `init`: Specify path to init (`path`)

### TTY Settings
- `tty.background_image`: Set background image for TTY (`path`)
- `tty.font`: Set custom font for TTY (`path`)
