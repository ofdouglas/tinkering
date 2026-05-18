/**
  ******************************************************************************
  * @file    log_c.h
  * @brief   C adapter: fatal logging only (HAL / FreeRTOS)
  ******************************************************************************
  */

#ifndef LOG_C_H
#define LOG_C_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void log_fatal_c(const char* file, uint32_t line, const char* msg, size_t msg_len)
    __attribute__((noreturn));

#define FATAL(msg) log_fatal_c(__FILE__, __LINE__, (msg), sizeof(msg) - 1U)
#define Error_Handler() FATAL("unknown")

#ifdef __cplusplus
}
#endif

#endif /* LOG_C_H */
