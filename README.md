# QMiniNVMeDevice

## Description
This is a subclass of the MiniPCIDevice library designed to communicate with NVMe devices over the MiniNVMe kernel module.

## Features
- General device info (ID and namespaces);
- Read and Write;
- Basic error handling;
- Timeout control;
- Controller reset;

## Pure C userspace library

To make the userspace program reusable from non-Qt projects, a pure C API is now provided:

- `src/mininvme_user.h`
- `src/mininvme_user.c`

This API mirrors the same mininvme ioctl flow used by `QMiniNVMeDevice`:

- open/close and scan `/dev/mininvme*`;
- controller version/state/info;
- namespace info and health log page;
- read/write by LBA;
- timeout set/get and controller reset;
- NVMe status decoding for device errors.

### Dependencies

The C API expects the mininvme driver userspace headers to be available, especially:

- `mininvme/ioctl.h`

It also uses `src/QMiniNVMeCommon.h` for NVMe data structures.

### Building in another program

Compile and link the source directly with your project (example):

```bash
cc -std=c11 -I/path/to/mininvme -I/path/to/QMiniNVMeDevice/src \
   your_app.c /path/to/QMiniNVMeDevice/src/mininvme_user.c -o your_app
```

### Minimal usage sketch

1. Initialize `mininvme_device_t` with `mininvme_device_init`.
2. Open `/dev/mininvmeX` with `mininvme_open`.
3. Call read/info/admin helpers as needed.
4. On failure, inspect `mininvme_last_error`, `mininvme_last_errno`, and `mininvme_last_nvme_error`.
5. Close with `mininvme_close`.

See MiniNVMeTestApp for more details.

![](/img/screenshot.png)
