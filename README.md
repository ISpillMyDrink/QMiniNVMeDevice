# QMiniNVMeDevice (C-only userspace library)

This repository provides a pure C userspace interface for the `mininvme` kernel driver.

The project is now command-line oriented only (no Qt GUI app).

## Goals

- Provide a small C library for NVMe operations via `mininvme` ioctls.
- Keep API surface stable and embeddable in other C/C++ applications.
- Expose deterministic error handling for ioctl, timeout, and NVMe completion status.
- Provide a CLI example that exercises the main workflow.

## Features

- Device discovery for `/dev/mininvme*`
- Device open/close lifecycle management
- Controller version/state/info retrieval
- Namespace info retrieval
- SMART / health log page retrieval
- LBA read/write operations
- Controller reset
- Timeout set/get
- Error translation to readable strings
- Convenience helpers for parsing controller/namespace/health structures

## Dependencies

This library requires userspace headers from the `mininvme` driver:

- `mininvme/ioctl.h` (or `../../mininvme/ioctl.h` fallback)

The library also ships NVMe data structure definitions in:

- `src/QMiniNVMeCommon.h`

## Build

### Build library + CLI example

```bash
make MININVME_INCLUDE=/absolute/path/to/mininvme
```

Artifacts:

- Static library: `build/lib/libmininvme_user.a`
- CLI example: `build/bin/mininvme_cli_example`

### Clean build artifacts

```bash
make clean
```

### Compile directly in another project

```bash
cc -std=c11 -I/path/to/mininvme -I/path/to/QMiniNVMeDevice/src \
  your_app.c /path/to/QMiniNVMeDevice/src/mininvme_user.c -o your_app
```

## Command-line usage

The repository includes:

- `examples/mininvme_cli_example.c`

Run:

```bash
./build/bin/mininvme_cli_example /dev/mininvme0
```

It prints:

- Controller version
- Model/firmware/serial
- Namespace summary
- Selected health metrics

## API overview (`src/mininvme_user.h`)

### Device lifecycle

- `mininvme_device_init`
- `mininvme_open`
- `mininvme_close`
- `mininvme_is_open`

### Error access

- `mininvme_last_error`
- `mininvme_last_errno`
- `mininvme_last_nvme_error`
- `mininvme_error_to_string`
- `mininvme_status_code_type_to_string`
- `mininvme_status_code_to_string`

### Device/admin commands

- `mininvme_controller_version`
- `mininvme_controller_state`
- `mininvme_controller_info`
- `mininvme_namespace_info`
- `mininvme_log_page_health_info`
- `mininvme_controller_reset`
- `mininvme_set_timeout`
- `mininvme_get_timeout`

### Data I/O

- `mininvme_read`
- `mininvme_write`

### Discovery helpers

- `mininvme_scan_devices`
- `mininvme_free_device_list`

### Structure parsing helpers

- Controller fields: model, firmware, serial, namespace count, max transfer, capacities
- Namespace fields: active LBA format / sector size
- Health fields: temperature and major counters

## Error model

Each API call returns `0` on success and `-1` on failure.

Failure details:

- `mininvme_last_error`:
  - `MININVME_ERROR_INVALID_ARGUMENT`
  - `MININVME_ERROR_OPEN`
  - `MININVME_ERROR_CLOSE`
  - `MININVME_ERROR_IOCTL`
  - `MININVME_ERROR_TIMEOUT`
  - `MININVME_ERROR_DEVICE`
- `mininvme_last_errno` for POSIX error context
- `mininvme_last_nvme_error` when `MININVME_ERROR_DEVICE` is set

## Project layout (detailed)

See:

- `docs/PROJECT_LAYOUT.md`
