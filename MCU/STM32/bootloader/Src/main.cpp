/**
  ******************************************************************************
  * @file    main.cpp
  * @brief   Bootloader main program
  ******************************************************************************
  */

#include "main.h"
#include "bsp.h"
#include "logging/log.hpp"
#include "bootloader/image_header.h"

#include <cstdint>

int main(void) {
    if (!bsp.earlyInit()) {
        LOG_FATAL() << "BSP initialization failed";
        return 1;
    }

    LOG_INFO() << "Bootloader started, magic=0x" << Bootloader::ImageHeader::kMagic;
    
    uint8_t byte{0U};
    for (;;) {
        bsp.toggleDebugLed();
        if (HAL_UART_Receive(&bsp.huart1, &byte, 1U, 10U) == HAL_OK) {
            (void)HAL_UART_Transmit(&bsp.huart1, &byte, 1U, 10U);
        }
    }
}
