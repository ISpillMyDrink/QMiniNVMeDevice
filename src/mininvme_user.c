#include "mininvme_user.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

static void clear_nvme_error(mininvme_device_t *device);

static int starts_with(const char *value, const char *prefix)
{
    while (*prefix) {
        if (*value == '\0' || *value != *prefix)
            return 0;
        value++;
        prefix++;
    }

    return 1;
}

static void set_last_error(mininvme_device_t *device, mininvme_error_t error, int err_no)
{
    device->last_error = error;
    device->last_errno = err_no;

    if (error != MININVME_ERROR_DEVICE)
        clear_nvme_error(device);
}

static void clear_nvme_error(mininvme_device_t *device)
{
    memset(&device->last_nvme_error, 0, sizeof(device->last_nvme_error));
}

static void set_last_error_from_status(mininvme_device_t *device, int err, const nvme_status_t *status)
{
    if (err < 0) {
        set_last_error(device, MININVME_ERROR_IOCTL, errno);
        clear_nvme_error(device);
        return;
    }

    if (status->timeout) {
        set_last_error(device, MININVME_ERROR_TIMEOUT, ETIMEDOUT);
        clear_nvme_error(device);
        return;
    }

    if (status->sct || status->sc) {
        set_last_error(device, MININVME_ERROR_DEVICE, 0);
        device->last_nvme_error.status_code_type = status->sct;
        device->last_nvme_error.status_code = status->sc;
        device->last_nvme_error.more = status->more ? 1 : 0;
        device->last_nvme_error.do_not_retry = status->dnr ? 1 : 0;
        return;
    }

    set_last_error(device, MININVME_ERROR_NONE, 0);
    clear_nvme_error(device);
}

void mininvme_device_init(mininvme_device_t *device)
{
    if (device == NULL)
        return;

    memset(device, 0, sizeof(*device));
    device->fd = -1;
}

int mininvme_open(mininvme_device_t *device, const char *device_path)
{
    if (device == NULL || device_path == NULL) {
        if (device != NULL)
            set_last_error(device, MININVME_ERROR_INVALID_ARGUMENT, EINVAL);
        return -1;
    }

    if (device->fd >= 0) {
        if (close(device->fd) != 0) {
            set_last_error(device, MININVME_ERROR_CLOSE, errno);
            return -1;
        }
    }

    device->fd = open(device_path, O_RDWR);
    if (device->fd < 0) {
        set_last_error(device, MININVME_ERROR_OPEN, errno);
        return -1;
    }

    set_last_error(device, MININVME_ERROR_NONE, 0);
    return 0;
}

int mininvme_close(mininvme_device_t *device)
{
    if (device == NULL) {
        errno = EINVAL;
        return -1;
    }

    if (device->fd < 0) {
        set_last_error(device, MININVME_ERROR_NONE, 0);
        return 0;
    }

    if (close(device->fd) != 0) {
        set_last_error(device, MININVME_ERROR_CLOSE, errno);
        return -1;
    }

    device->fd = -1;
    set_last_error(device, MININVME_ERROR_NONE, 0);
    return 0;
}

int mininvme_is_open(const mininvme_device_t *device)
{
    if (device == NULL)
        return 0;

    return device->fd >= 0 ? 1 : 0;
}

mininvme_error_t mininvme_last_error(const mininvme_device_t *device)
{
    if (device == NULL)
        return MININVME_ERROR_INVALID_ARGUMENT;

    return device->last_error;
}

int mininvme_last_errno(const mininvme_device_t *device)
{
    if (device == NULL)
        return EINVAL;

    return device->last_errno;
}

mininvme_nvme_error_t mininvme_last_nvme_error(const mininvme_device_t *device)
{
    mininvme_nvme_error_t empty = {0, 0, 0, 0};

    if (device == NULL)
        return empty;

    return device->last_nvme_error;
}

int mininvme_controller_version(mininvme_device_t *device, nvme_controller_version_t *version)
{
    if (device == NULL || version == NULL || device->fd < 0) {
        if (device != NULL)
            set_last_error(device, MININVME_ERROR_INVALID_ARGUMENT, EINVAL);
        return -1;
    }

    memset(version, 0, sizeof(*version));
    if (ioctl(device->fd, NVME_IOCTL_GET_CONTROLLER_VERSION, version) < 0) {
        set_last_error(device, MININVME_ERROR_IOCTL, errno);
        return -1;
    }

    set_last_error(device, MININVME_ERROR_NONE, 0);
    return 0;
}

int mininvme_controller_state(mininvme_device_t *device, nvme_controller_state_t *state)
{
    if (device == NULL || state == NULL || device->fd < 0) {
        if (device != NULL)
            set_last_error(device, MININVME_ERROR_INVALID_ARGUMENT, EINVAL);
        return -1;
    }

    memset(state, 0, sizeof(*state));
    if (ioctl(device->fd, NVME_IOCTL_GET_CONTROLLER_STATE, state) < 0) {
        set_last_error(device, MININVME_ERROR_IOCTL, errno);
        return -1;
    }

    set_last_error(device, MININVME_ERROR_NONE, 0);
    return 0;
}

int mininvme_controller_info(mininvme_device_t *device, nvme_controller_info_t *info)
{
    nvme_command_packet_t packet;
    int err;

    if (device == NULL || info == NULL || device->fd < 0) {
        if (device != NULL)
            set_last_error(device, MININVME_ERROR_INVALID_ARGUMENT, EINVAL);
        return -1;
    }

    memset(info, 0, sizeof(*info));
    memset(&packet, 0, sizeof(packet));
    packet.cmd.opc = NVME_ADMIN_IDENTIFY;
    packet.cmd.cdw10 = NVME_ADMIN_IDENTIFY_CONTROLLER;
    packet.buffer.pointer = (uint8_t *)info;
    packet.buffer.length = sizeof(*info);

    err = ioctl(device->fd, NVME_IOCTL_RUN_ADMIN_COMMAND, &packet);
    set_last_error_from_status(device, err, &packet.status);
    return device->last_error == MININVME_ERROR_NONE ? 0 : -1;
}

int mininvme_namespace_info(mininvme_device_t *device, uint32_t nsid, nvme_namespace_info_t *info)
{
    nvme_command_packet_t packet;
    int err;

    if (device == NULL || info == NULL || nsid == 0 || device->fd < 0) {
        if (device != NULL)
            set_last_error(device, MININVME_ERROR_INVALID_ARGUMENT, EINVAL);
        return -1;
    }

    memset(info, 0, sizeof(*info));
    memset(&packet, 0, sizeof(packet));
    packet.cmd.nsid = nsid;
    packet.cmd.opc = NVME_ADMIN_IDENTIFY;
    packet.cmd.cdw10 = NVME_ADMIN_IDENTIFY_NAMESPACE;
    packet.buffer.pointer = (uint8_t *)info;
    packet.buffer.length = sizeof(*info);

    err = ioctl(device->fd, NVME_IOCTL_RUN_ADMIN_COMMAND, &packet);
    set_last_error_from_status(device, err, &packet.status);
    return device->last_error == MININVME_ERROR_NONE ? 0 : -1;
}

int mininvme_log_page_health_info(mininvme_device_t *device, nvme_log_page_health_information_t *info)
{
    nvme_command_packet_t packet;
    nvme_log_page_cdw10_t *cdw10;
    int err;

    if (device == NULL || info == NULL || device->fd < 0) {
        if (device != NULL)
            set_last_error(device, MININVME_ERROR_INVALID_ARGUMENT, EINVAL);
        return -1;
    }

    memset(info, 0, sizeof(*info));
    memset(&packet, 0, sizeof(packet));
    packet.cmd.nsid = (uint32_t)-1;
    packet.cmd.opc = NVME_ADMIN_GET_LOG_PAGE;

    cdw10 = (nvme_log_page_cdw10_t *)&packet.cmd.cdw10;
    cdw10->lid = NVME_ADMIN_GET_LOG_PAGE_HEALTH_INFORMATION;
    cdw10->numd = sizeof(*info) / sizeof(uint32_t);

    packet.buffer.pointer = (uint8_t *)info;
    packet.buffer.length = sizeof(*info);

    err = ioctl(device->fd, NVME_IOCTL_RUN_ADMIN_COMMAND, &packet);
    set_last_error_from_status(device, err, &packet.status);
    return device->last_error == MININVME_ERROR_NONE ? 0 : -1;
}

int mininvme_read(mininvme_device_t *device, uint64_t offset, uint32_t count, void *buffer, uint32_t length, uint32_t nsid)
{
    nvme_lba_packet_t packet;
    int err;

    if (device == NULL || buffer == NULL || count == 0 || length == 0 || nsid == 0 || device->fd < 0) {
        if (device != NULL)
            set_last_error(device, MININVME_ERROR_INVALID_ARGUMENT, EINVAL);
        return -1;
    }

    memset(&packet, 0, sizeof(packet));
    packet.nsid = nsid;
    packet.lba.offset = offset;
    packet.lba.count = count;
    packet.buffer.pointer = (uint8_t *)buffer;
    packet.buffer.length = length;

    err = ioctl(device->fd, NVME_IOCTL_READ_SECTORS, &packet);
    set_last_error_from_status(device, err, &packet.status);
    return device->last_error == MININVME_ERROR_NONE ? 0 : -1;
}

int mininvme_write(mininvme_device_t *device, uint64_t offset, uint32_t count, const void *buffer, uint32_t length, uint32_t nsid)
{
    nvme_lba_packet_t packet;
    int err;

    if (device == NULL || buffer == NULL || count == 0 || length == 0 || nsid == 0 || device->fd < 0) {
        if (device != NULL)
            set_last_error(device, MININVME_ERROR_INVALID_ARGUMENT, EINVAL);
        return -1;
    }

    memset(&packet, 0, sizeof(packet));
    packet.nsid = nsid;
    packet.lba.offset = offset;
    packet.lba.count = count;
    packet.buffer.pointer = (uint8_t *)buffer;
    packet.buffer.length = length;

    err = ioctl(device->fd, NVME_IOCTL_WRITE_SECTORS, &packet);
    set_last_error_from_status(device, err, &packet.status);
    return device->last_error == MININVME_ERROR_NONE ? 0 : -1;
}

int mininvme_controller_reset(mininvme_device_t *device)
{
    if (device == NULL || device->fd < 0) {
        if (device != NULL)
            set_last_error(device, MININVME_ERROR_INVALID_ARGUMENT, EINVAL);
        return -1;
    }

    if (ioctl(device->fd, NVME_IOCTL_CONTROLLER_RESET) < 0) {
        set_last_error(device, MININVME_ERROR_IOCTL, errno);
        return -1;
    }

    set_last_error(device, MININVME_ERROR_NONE, 0);
    return 0;
}

int mininvme_set_timeout(mininvme_device_t *device, int value)
{
    nvme_timeout_t timeout;

    if (device == NULL || device->fd < 0) {
        if (device != NULL)
            set_last_error(device, MININVME_ERROR_INVALID_ARGUMENT, EINVAL);
        return -1;
    }

    timeout.value = value;
    if (ioctl(device->fd, NVME_IOCTL_SET_TIMOUT, &timeout) < 0) {
        set_last_error(device, MININVME_ERROR_IOCTL, errno);
        return -1;
    }

    set_last_error(device, MININVME_ERROR_NONE, 0);
    return 0;
}

int mininvme_get_timeout(mininvme_device_t *device, int *value)
{
    nvme_timeout_t timeout;

    if (device == NULL || value == NULL || device->fd < 0) {
        if (device != NULL)
            set_last_error(device, MININVME_ERROR_INVALID_ARGUMENT, EINVAL);
        return -1;
    }

    if (ioctl(device->fd, NVME_IOCTL_GET_TIMOUT, &timeout) < 0) {
        set_last_error(device, MININVME_ERROR_IOCTL, errno);
        return -1;
    }

    *value = timeout.value;
    set_last_error(device, MININVME_ERROR_NONE, 0);
    return 0;
}

int mininvme_scan_devices(char **out_paths, size_t max_paths, size_t *out_count)
{
    DIR *dir;
    struct dirent *entry;
    size_t count = 0;

    if (out_paths == NULL || out_count == NULL) {
        errno = EINVAL;
        return -1;
    }

    *out_count = 0;
    dir = opendir("/dev");
    if (dir == NULL)
        return -1;

    while ((entry = readdir(dir)) != NULL) {
        char *path;
        size_t path_len;

        if (!starts_with(entry->d_name, "mininvme"))
            continue;

        if (count >= max_paths) {
            mininvme_free_device_list(out_paths, count);
            closedir(dir);
            errno = ENOSPC;
            return -1;
        }

        path_len = strlen(entry->d_name) + 6;
        path = (char *)malloc(path_len);
        if (path == NULL) {
            mininvme_free_device_list(out_paths, count);
            closedir(dir);
            errno = ENOMEM;
            return -1;
        }

        (void)snprintf(path, path_len, "/dev/%s", entry->d_name);
        out_paths[count] = path;
        count++;
    }

    closedir(dir);
    *out_count = count;
    return 0;
}

void mininvme_free_device_list(char **paths, size_t count)
{
    size_t index;

    if (paths == NULL)
        return;

    for (index = 0; index < count; index++) {
        free(paths[index]);
        paths[index] = NULL;
    }
}

const char *mininvme_error_to_string(mininvme_error_t error)
{
    switch (error) {
    case MININVME_ERROR_NONE:
        return "No error";
    case MININVME_ERROR_INVALID_ARGUMENT:
        return "Invalid argument";
    case MININVME_ERROR_OPEN:
        return "Device open error";
    case MININVME_ERROR_CLOSE:
        return "Device close error";
    case MININVME_ERROR_IOCTL:
        return "Device ioctl error";
    case MININVME_ERROR_TIMEOUT:
        return "Device timeout";
    case MININVME_ERROR_DEVICE:
        return "NVMe device error";
    default:
        return "Unknown error";
    }
}

const char *mininvme_status_code_type_to_string(int type)
{
    switch (type) {
    case 0x00:
        return "Generic command status";
    case 0x01:
        return "Command specific status";
    case 0x02:
        return "Media specific or data integrity error";
    default:
        return "Unknown status code type";
    }
}

const char *mininvme_status_code_to_string(int type, int code)
{
    if (type == 0x00) {
        switch (code) {
        case 0x00: return "Success";
        case 0x01: return "Invalid command opcode";
        case 0x02: return "Invalid field in command";
        case 0x03: return "Command ID conflict";
        case 0x04: return "Data transfer error";
        case 0x05: return "Commands aborted due to power loss notification";
        case 0x06: return "Internal device error";
        case 0x07: return "Command abort requested";
        case 0x08: return "Command aborted due to SQ deletion";
        case 0x09: return "Command aborted due to failed fused command";
        case 0x0a: return "Command aborted due to missing fused command";
        case 0x0b: return "Invalid namespace or format";
        case 0x0c: return "Command sequence error";
        case 0x80: return "LBA out of range";
        case 0x81: return "Capacity exceeded";
        case 0x82: return "Namespace not ready";
        default: return "Unknown status code";
        }
    }

    if (type == 0x01) {
        switch (code) {
        case 0x00: return "Completion queue invalid";
        case 0x01: return "Invalid queue identifier";
        case 0x02: return "Maximum queue size exceeded";
        case 0x03: return "Abort command limit exceeded";
        case 0x05: return "Asynchronous event request limit exceeded";
        case 0x06: return "Invalid firmware slot";
        case 0x07: return "Invalid firmware image";
        case 0x08: return "Invalid interrupt vector";
        case 0x09: return "Invalid log page";
        case 0x0a: return "Invalid format";
        case 0x0b: return "Firmware application requires conventional reset";
        case 0x0c: return "Invalid queue deletion";
        case 0x80: return "Conflicting attributes";
        case 0x81: return "Invalid protection information";
        case 0x82: return "Attempted write to read only range";
        default: return "Unknown status code";
        }
    }

    if (type == 0x02) {
        switch (code) {
        case 0x80: return "Write fault";
        case 0x81: return "Unrecovered read error";
        case 0x82: return "End-to-end guard check error";
        case 0x83: return "End-to-end application tag check error";
        case 0x84: return "End-to-end reference tag check error";
        case 0x85: return "Compare failure";
        case 0x86: return "Access denied";
        default: return "Unknown status code";
        }
    }

    return "Unknown status code";
}

static void copy_trimmed_ascii(const char *src, size_t src_len, char *out, size_t out_size)
{
    size_t start = 0;
    size_t end = src_len;
    size_t len;

    if (out == NULL || out_size == 0)
        return;

    while (start < src_len && src[start] == ' ')
        start++;

    while (end > start && src[end - 1] == ' ')
        end--;

    len = end - start;
    if (len >= out_size)
        len = out_size - 1;

    if (len > 0)
        memcpy(out, src + start, len);
    out[len] = '\0';
}

void mininvme_controller_model_name(const nvme_controller_info_t *info, char *out, size_t out_size)
{
    if (info == NULL || out == NULL || out_size == 0)
        return;
    copy_trimmed_ascii(info->mn, sizeof(info->mn), out, out_size);
}

void mininvme_controller_firmware_revision(const nvme_controller_info_t *info, char *out, size_t out_size)
{
    if (info == NULL || out == NULL || out_size == 0)
        return;
    copy_trimmed_ascii(info->fr, sizeof(info->fr), out, out_size);
}

void mininvme_controller_serial_number(const nvme_controller_info_t *info, char *out, size_t out_size)
{
    if (info == NULL || out == NULL || out_size == 0)
        return;
    copy_trimmed_ascii(info->sn, sizeof(info->sn), out, out_size);
}

uint32_t mininvme_controller_namespace_count(const nvme_controller_info_t *info)
{
    if (info == NULL)
        return 0;
    return info->nn;
}

uint32_t mininvme_controller_max_data_transfer_size(const nvme_controller_info_t *info)
{
    if (info == NULL)
        return 0;

    if (info->mdts >= 20)
        return 0;

    return 4096U * (1U << info->mdts);
}

uint64_t mininvme_controller_total_capacity(const nvme_controller_info_t *info)
{
    if (info == NULL)
        return 0;
    return info->tnvmcap_lo;
}

uint64_t mininvme_controller_unallocated_capacity(const nvme_controller_info_t *info)
{
    if (info == NULL)
        return 0;
    return info->unvmcap_lo;
}

uint16_t mininvme_namespace_sector_size(const nvme_namespace_info_t *info)
{
    uint8_t flbas;

    if (info == NULL)
        return 0;

    flbas = info->flbas & 0x0f;
    if (flbas >= 16)
        return 0;

    return (uint16_t)(1U << info->lbaf[flbas].lbads);
}

uint16_t mininvme_health_composite_temperature(const nvme_log_page_health_information_t *info)
{
    if (info == NULL)
        return 0;
    return (uint16_t)(info->ct[0] | (info->ct[1] << 8));
}

uint64_t mininvme_health_data_units_read(const nvme_log_page_health_information_t *info)
{
    if (info == NULL)
        return 0;
    return info->dur_lo;
}

uint64_t mininvme_health_data_units_written(const nvme_log_page_health_information_t *info)
{
    if (info == NULL)
        return 0;
    return info->duw_lo;
}

uint64_t mininvme_health_host_read_commands(const nvme_log_page_health_information_t *info)
{
    if (info == NULL)
        return 0;
    return info->hrc_lo;
}

uint64_t mininvme_health_host_write_commands(const nvme_log_page_health_information_t *info)
{
    if (info == NULL)
        return 0;
    return info->hwc_lo;
}

uint64_t mininvme_health_controller_busy_time(const nvme_log_page_health_information_t *info)
{
    if (info == NULL)
        return 0;
    return info->cbt_lo;
}

uint64_t mininvme_health_power_cycles(const nvme_log_page_health_information_t *info)
{
    if (info == NULL)
        return 0;
    return info->pc_lo;
}

uint64_t mininvme_health_power_on_hours(const nvme_log_page_health_information_t *info)
{
    if (info == NULL)
        return 0;
    return info->poh_lo;
}

uint64_t mininvme_health_unsafe_shutdowns(const nvme_log_page_health_information_t *info)
{
    if (info == NULL)
        return 0;
    return info->us_lo;
}

uint64_t mininvme_health_media_and_data_integrity_errors(const nvme_log_page_health_information_t *info)
{
    if (info == NULL)
        return 0;
    return info->mdie_lo;
}

uint64_t mininvme_health_number_of_error_information_log_entries(const nvme_log_page_health_information_t *info)
{
    if (info == NULL)
        return 0;
    return info->neile_lo;
}
