# Modules with a <name>_test.cpp source file.
MODULES := hdlc crc

hdlc_SRCS := hdlc/hdlc_test.cpp hdlc/hdlc.cpp
hdlc_TEST := hdlc_test

crc_SRCS := crc/crc_test.cpp crc/crc.cpp
crc_TEST := crc_test
