/**
  ******************************************************************************
  * @file    error.h
  * @brief   Fatal error reporting (C-safe macros + error_halt)
  ******************************************************************************
  */

#ifndef ERROR_H
#define ERROR_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void error_halt(const char *file, uint32_t line, const char *msg, size_t msg_len);

/** Log file:line and a string literal message, then halt. */
#define ERROR(msg) error_halt(__FILE__, __LINE__, (msg), sizeof(msg) - 1U)

/** HAL / legacy call sites with no message. */
#define Error_Handler() ERROR("unknown")

#ifdef __cplusplus
}
#endif

#endif /* ERROR_H */
