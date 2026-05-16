/**
  ******************************************************************************
  * @file    error.cpp
  * @brief   Fatal error halt with UART diagnostics
  ******************************************************************************
  */

#include "error.h"

#include "bsp.h"

#include <cstdio>

namespace {

const char *fileBasename(const char *path)
{
  const char *base = path;
  for (const char *p = path; *p != '\0'; ++p)
  {
    if (*p == '/' || *p == '\\')
    {
      base = p + 1;
    }
  }
  return base;
}

bool uartReady()
{
  return bsp.huart1.Instance != nullptr &&
         bsp.huart1.gState != HAL_UART_STATE_RESET;
}

} /* namespace */

extern "C" void error_halt(const char *file, uint32_t line, const char *msg, size_t msg_len)
{
  __disable_irq();

  char buf[128];
  const int n = std::snprintf(buf, sizeof(buf), "%s:%lu %.*s\n", fileBasename(file),
                              static_cast<unsigned long>(line), static_cast<int>(msg_len), msg);

  if (n > 0 && uartReady())
  {
    const uint16_t len = static_cast<uint16_t>((n < static_cast<int>(sizeof(buf))) ? n : sizeof(buf) - 1U);
    (void)bsp.uartTransmit(reinterpret_cast<const uint8_t *>(buf), len, 10000U);
  }

  while (true)
  {
    bsp.ledToggle();
    for (volatile uint32_t i = 0; i < 500000U; ++i)
    {
    }
  }
}
