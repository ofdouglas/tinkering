# ARM GCC toolchain detection for STM32 builds.

CUBE_GCC_WIN ?= C:/ST/STM32CubeIDE_2.0.0/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.13.3.rel1.win32_1.0.100.202509120712/tools/bin

ifdef GCC_PATH
  GNU_BIN = $(GCC_PATH)/
  TOOL_SUFFIX = $(if $(wildcard $(GNU_BIN)arm-none-eabi-gcc.exe),.exe,)
else ifneq ($(wildcard $(CUBE_GCC_WIN)/arm-none-eabi-gcc.exe),)
  GNU_BIN = $(CUBE_GCC_WIN)/
  TOOL_SUFFIX = .exe
else ifneq ($(shell which arm-none-eabi-gcc 2>/dev/null),)
  GNU_BIN =
  TOOL_SUFFIX =
else
  $(error arm-none-eabi-gcc not found — install gcc-arm-none-eabi or set GCC_PATH)
endif

CC  = $(GNU_BIN)arm-none-eabi-gcc$(TOOL_SUFFIX)
CXX = $(GNU_BIN)arm-none-eabi-g++$(TOOL_SUFFIX)
AS  = $(GNU_BIN)arm-none-eabi-gcc$(TOOL_SUFFIX)
ifeq ($(STM32_LINK_CXX),1)
LD  = $(CXX)
else
LD  = $(CC)
endif
SZ  = $(GNU_BIN)arm-none-eabi-size$(TOOL_SUFFIX)
MKDIR = mkdir -p
