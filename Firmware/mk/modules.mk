# Modules with a <name>_test.cpp source file.
MODULES := hdlc crc bootloader

# Link in host unit tests instead of platform logging/clock sources.
HOST_TEST_SUPPORT_SRCS := hal/mock_clocks.cpp logging/mock_log.cpp

hdlc_SRCS := hdlc/hdlc_test.cpp hdlc/hdlc.cpp
hdlc_TEST := hdlc_test

crc_SRCS := crc/crc_test.cpp crc/crc.cpp
crc_TEST := crc_test

bootloader_SRCS := \
	bootloader/test/bootloader_test.cpp \
	bootloader/test/mocks.cpp \
	bootloader/mcu/bootloader.cpp \
	$(HOST_TEST_SUPPORT_SRCS)
bootloader_TEST := bootloader_test
