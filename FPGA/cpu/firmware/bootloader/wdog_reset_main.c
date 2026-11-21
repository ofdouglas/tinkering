#include "drivers/watchdog.h"

/*
 * Throwaway bootloader for system_tb watchdog-reset bring-up.
 * Configures a short timeout and does not service the watchdog.
 */
int main(void) {
    watchdog_configure(0, 7);
    watchdog_start();
    for (;;) {
    }
}
