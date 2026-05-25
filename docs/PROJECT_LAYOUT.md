# Project Layout

This document describes every tracked source area in the repository after the C-only migration.

## Root

- `LICENSE`
  - Project license text (GPL v2).

- `README.md`
  - Main entrypoint: capabilities, build, CLI usage, API overview, and error model.

- `Makefile`
  - Default build for C library and CLI example.
  - Important variables:
    - `CC`, `CFLAGS`, `AR`, `ARFLAGS`
    - `MININVME_INCLUDE` (path to mininvme userspace headers)
  - Main targets:
    - `all` (builds `lib` + `example`)
    - `lib`
    - `example`
    - `clean`

## `src/`

- `mininvme_user.h`
  - Public API header.
  - Includes:
    - mininvme ioctl declarations (`mininvme/ioctl.h` include + fallback)
    - NVMe payload structures (`QMiniNVMeCommon.h`)
  - Defines:
    - `mininvme_error_t`
    - `mininvme_nvme_error_t`
    - `mininvme_device_t`
  - Declares all public functions:
    - lifecycle
    - admin/data I/O operations
    - timeout/reset
    - device scanning helpers
    - data extraction helpers
    - error/status string mappers

- `mininvme_user.c`
  - Concrete implementation of the public API.
  - Internals:
    - local error state handling helpers
    - status-to-error conversion for ioctl packet completion status
    - `/dev` directory scanning for `mininvme*` devices
  - Implements:
    - IOCTL command wrappers (admin + I/O)
    - discovery and memory ownership helpers
    - parse/format helper utilities
    - NVMe status code translation tables

- `QMiniNVMeCommon.h`
  - Shared NVMe command constants and packed payload structures.
  - Contains static size checks for major on-wire structures:
    - `nvme_controller_info_t` (4096 bytes)
    - `nvme_namespace_info_t` (4096 bytes)
    - `nvme_log_page_cdw10_t` (4 bytes)
    - `nvme_log_page_error_information_entry_t` (64 bytes)
    - `nvme_log_page_health_information_t` (512 bytes)
  - C-compatible `static_assert` mapping is provided for non-C++ compilation.

## `examples/`

- `mininvme_cli_example.c`
  - Command-line-only demonstration program.
  - Flow:
    1. open device path from CLI arg
    2. fetch controller version + identify info
    3. print namespace details
    4. fetch selected health fields
    5. close device
  - Shows canonical error-reporting path using:
    - `mininvme_last_error`
    - `mininvme_last_errno`
    - `mininvme_last_nvme_error`

## Build output (generated, not tracked)

- `build/obj/`
  - object files
- `build/lib/`
  - static library (`libmininvme_user.a`)
- `build/bin/`
  - CLI example binary (`mininvme_cli_example`)

## External integration assumptions

This repository intentionally does not vendor the `mininvme` kernel/userspace header tree.
Consumers must make `mininvme/ioctl.h` visible through include paths.
