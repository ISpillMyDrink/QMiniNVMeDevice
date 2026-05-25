#ifndef MININVME_USER_H
#define MININVME_USER_H

#include <stddef.h>
#include <stdint.h>

#if defined(__has_include)
#if __has_include(<mininvme/ioctl.h>)
#include <mininvme/ioctl.h>
#elif __has_include("../../mininvme/ioctl.h")
#include "../../mininvme/ioctl.h"
#else
#error "Cannot find mininvme/ioctl.h. Add the mininvme userspace headers to your include path."
#endif
#else
#include "../../mininvme/ioctl.h"
#endif
#include "QMiniNVMeCommon.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MININVME_ERROR_NONE = 0,
    MININVME_ERROR_INVALID_ARGUMENT,
    MININVME_ERROR_OPEN,
    MININVME_ERROR_CLOSE,
    MININVME_ERROR_IOCTL,
    MININVME_ERROR_TIMEOUT,
    MININVME_ERROR_DEVICE
} mininvme_error_t;

typedef struct {
    int status_code_type;
    int status_code;
    int more;
    int do_not_retry;
} mininvme_nvme_error_t;

typedef struct {
    int fd;
    mininvme_error_t last_error;
    int last_errno;
    mininvme_nvme_error_t last_nvme_error;
} mininvme_device_t;

void mininvme_device_init(mininvme_device_t *device);
int mininvme_open(mininvme_device_t *device, const char *device_path);
int mininvme_close(mininvme_device_t *device);
int mininvme_is_open(const mininvme_device_t *device);

mininvme_error_t mininvme_last_error(const mininvme_device_t *device);
int mininvme_last_errno(const mininvme_device_t *device);
mininvme_nvme_error_t mininvme_last_nvme_error(const mininvme_device_t *device);

int mininvme_controller_version(mininvme_device_t *device, nvme_controller_version_t *version);
int mininvme_controller_state(mininvme_device_t *device, nvme_controller_state_t *state);
int mininvme_controller_info(mininvme_device_t *device, nvme_controller_info_t *info);
int mininvme_namespace_info(mininvme_device_t *device, uint32_t nsid, nvme_namespace_info_t *info);
int mininvme_log_page_health_info(mininvme_device_t *device, nvme_log_page_health_information_t *info);

int mininvme_read(mininvme_device_t *device, uint64_t offset, uint32_t count, void *buffer, uint32_t length, uint32_t nsid);
int mininvme_write(mininvme_device_t *device, uint64_t offset, uint32_t count, const void *buffer, uint32_t length, uint32_t nsid);

int mininvme_controller_reset(mininvme_device_t *device);
int mininvme_set_timeout(mininvme_device_t *device, int value);
int mininvme_get_timeout(mininvme_device_t *device, int *value);

int mininvme_scan_devices(char **out_paths, size_t max_paths, size_t *out_count);
void mininvme_free_device_list(char **paths, size_t count);

const char *mininvme_error_to_string(mininvme_error_t error);
const char *mininvme_status_code_type_to_string(int type);
const char *mininvme_status_code_to_string(int type, int code);

#ifdef __cplusplus
}
#endif

#endif // MININVME_USER_H
