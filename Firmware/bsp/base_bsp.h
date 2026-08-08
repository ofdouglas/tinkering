#ifndef BSP_BASE_BSP_H
#define BSP_BASE_BSP_H

#include <cstdint>
#include <cstddef>
#include "logging/logging.h"
#include "util/span.h"

class BaseBsp {
public:
    /**
     * @brief  Initialize the platform clock and debug UART.
     * @return True if the initialization is successful, false otherwise.
     */
    virtual bool earlyInit() noexcept = 0;

    /**
     * @brief  Set the debug LED.
     * @param on True if the LED should be turned on, false otherwise.
     */
    virtual void setDebugLed(bool on) noexcept = 0;

    virtual void toggleDebugLed() noexcept = 0;
};


#endif // BSP_BASE_BSP_H