#define _POSIX_C_SOURCE 200809L

#include "bootloader/host/platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

void host_idle_wait(void) {
    const struct timespec ts = {0, 200000L};
    nanosleep(&ts, NULL);
}

void host_boot_app(const uint8_t* image, size_t length) {
    const char* dump_path = getenv("HOST_BL_IMAGE_OUT");
    if (dump_path != NULL) {
        FILE* dump_file = fopen(dump_path, "wb");
        if (dump_file == NULL) {
            fprintf(stderr, "host bootloader: could not write %s\n", dump_path);
            _exit(1);
        }
        const size_t written = fwrite(image, 1U, length, dump_file);
        const int close_rc = fclose(dump_file);
        if ((written != length) || (close_rc != 0)) {
            fprintf(stderr, "host bootloader: short write to %s\n", dump_path);
            _exit(1);
        }
    }
    _exit(0);
}
