#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "../src/mininvme_user.h"

static void print_last_error(const mininvme_device_t *device)
{
    mininvme_error_t err = mininvme_last_error(device);
    fprintf(stderr, "Error: %s", mininvme_error_to_string(err));

    if (err == MININVME_ERROR_DEVICE) {
        mininvme_nvme_error_t nvme_err = mininvme_last_nvme_error(device);
        fprintf(stderr, " (%s: %s)",
                mininvme_status_code_type_to_string(nvme_err.status_code_type),
                mininvme_status_code_to_string(nvme_err.status_code_type, nvme_err.status_code));
    } else if (mininvme_last_errno(device) != 0) {
        fprintf(stderr, " (errno=%d)", mininvme_last_errno(device));
    }

    fprintf(stderr, "\n");
}

int main(int argc, char **argv)
{
    mininvme_device_t device;
    nvme_controller_version_t version;
    nvme_controller_info_t controller;
    nvme_namespace_info_t ns;
    nvme_log_page_health_information_t health;
    char model[41];
    char fw[9];
    char sn[21];
    uint32_t nsid;
    const char *path;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s /dev/mininvme0\n", argv[0]);
        return 1;
    }

    path = argv[1];
    mininvme_device_init(&device);

    if (mininvme_open(&device, path) != 0) {
        print_last_error(&device);
        return 1;
    }

    if (mininvme_controller_version(&device, &version) != 0) {
        print_last_error(&device);
        mininvme_close(&device);
        return 1;
    }

    if (mininvme_controller_info(&device, &controller) != 0) {
        print_last_error(&device);
        mininvme_close(&device);
        return 1;
    }

    mininvme_controller_model_name(&controller, model, sizeof(model));
    mininvme_controller_firmware_revision(&controller, fw, sizeof(fw));
    mininvme_controller_serial_number(&controller, sn, sizeof(sn));

    printf("Controller Version: %u.%u.%u\n", version.major, version.minor, version.tertiary);
    printf("Model: %s\n", model);
    printf("Firmware: %s\n", fw);
    printf("Serial: %s\n", sn);
    printf("Namespaces: %u\n", mininvme_controller_namespace_count(&controller));
    printf("Max transfer size: %u bytes\n", mininvme_controller_max_data_transfer_size(&controller));
    printf("Total capacity (low 64): %llu bytes\n",
           (unsigned long long)mininvme_controller_total_capacity(&controller));

    for (nsid = 1; nsid <= mininvme_controller_namespace_count(&controller); nsid++) {
        if (mininvme_namespace_info(&device, nsid, &ns) != 0) {
            print_last_error(&device);
            mininvme_close(&device);
            return 1;
        }

        printf("Namespace %u: size=%llu sectors, sector=%u bytes\n",
               nsid,
               (unsigned long long)ns.nsze,
               mininvme_namespace_sector_size(&ns));
    }

    if (mininvme_log_page_health_info(&device, &health) != 0) {
        print_last_error(&device);
        mininvme_close(&device);
        return 1;
    }

    printf("Composite temperature: %u K\n", mininvme_health_composite_temperature(&health));
    printf("Data units read: %llu\n",
           (unsigned long long)mininvme_health_data_units_read(&health));
    printf("Data units written: %llu\n",
           (unsigned long long)mininvme_health_data_units_written(&health));

    if (mininvme_close(&device) != 0) {
        print_last_error(&device);
        return 1;
    }

    return 0;
}
