# Source list for command-line builds (Debug configuration).
# Derived from STM32CubeIDE Debug/objects.list / subdir.mk — update after
# adding/removing files in CubeIDE and rebuilding once.

C_SOURCES = \
	Src/bsp_driver_sd.c \
	Src/fatfs.c \
	Src/fatfs_platform.c \
	Src/freertos.c \
	Src/main.c \
	Src/sd_diskio.c \
	Src/stm32f7xx_hal_msp.c \
	Src/stm32f7xx_hal_timebase_tim.c \
	Src/stm32f7xx_it.c \
	Src/syscalls.c \
	Src/sysmem.c \
	Src/system_stm32f7xx.c \
	Src/usb_host.c \
	Src/usbh_conf.c \
	Src/usbh_platform.c \
	Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal.c \
	Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_adc.c \
	Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_adc_ex.c \
	Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_cortex.c \
	Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_crc.c \
	Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_crc_ex.c \
	Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_dcmi.c \
	Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_dcmi_ex.c \
	Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_dma.c \
	Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_dma2d.c \
	Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_dma_ex.c \
	Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_dsi.c \
	Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_eth.c \
	Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_exti.c \
	Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_flash.c \
	Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_flash_ex.c \
	Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_gpio.c \
	Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_hcd.c \
	Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_i2c.c \
	Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_i2c_ex.c \
	Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_ltdc.c \
	Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_ltdc_ex.c \
	Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_mmc.c \
	Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_nand.c \
	Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_nor.c \
	Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_pwr.c \
	Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_pwr_ex.c \
	Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_qspi.c \
	Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_rcc.c \
	Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_rcc_ex.c \
	Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_rtc.c \
	Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_rtc_ex.c \
	Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_sai.c \
	Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_sai_ex.c \
	Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_sd.c \
	Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_sdram.c \
	Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_spdifrx.c \
	Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_spi.c \
	Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_spi_ex.c \
	Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_sram.c \
	Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_tim.c \
	Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_tim_ex.c \
	Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_uart.c \
	Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_uart_ex.c \
	Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_ll_fmc.c \
	Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_ll_sdmmc.c \
	Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_ll_usb.c \
	Middlewares/ST/STM32_USB_Host_Library/Class/CDC/Src/usbh_cdc.c \
	Middlewares/ST/STM32_USB_Host_Library/Core/Src/usbh_core.c \
	Middlewares/ST/STM32_USB_Host_Library/Core/Src/usbh_ctlreq.c \
	Middlewares/ST/STM32_USB_Host_Library/Core/Src/usbh_ioreq.c \
	Middlewares/ST/STM32_USB_Host_Library/Core/Src/usbh_pipes.c \
	Middlewares/Third_Party/FatFs/src/diskio.c \
	Middlewares/Third_Party/FatFs/src/ff.c \
	Middlewares/Third_Party/FatFs/src/ff_gen_drv.c \
	Middlewares/Third_Party/FatFs/src/option/syscall.c \
	Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS/cmsis_os.c \
	Middlewares/Third_Party/FreeRTOS/Source/croutine.c \
	Middlewares/Third_Party/FreeRTOS/Source/event_groups.c \
	Middlewares/Third_Party/FreeRTOS/Source/list.c \
	Middlewares/Third_Party/FreeRTOS/Source/queue.c \
	Middlewares/Third_Party/FreeRTOS/Source/stream_buffer.c \
	Middlewares/Third_Party/FreeRTOS/Source/tasks.c \
	Middlewares/Third_Party/FreeRTOS/Source/timers.c \
	Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM7/r0p1/port.c \
	Middlewares/Third_Party/FreeRTOS/Source/portable/MemMang/heap_4.c

ASM_SOURCES = \
	Startup/startup_stm32f746nghx.s
