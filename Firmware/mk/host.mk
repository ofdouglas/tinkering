# Shared host C++ build rules. Included from Firmware/Makefile.

ifndef FIRMWARE_ROOT
$(error FIRMWARE_ROOT must be set before including mk/host.mk)
endif

include $(FIRMWARE_ROOT)/mk/modules.mk

BUILD_DIR ?= $(FIRMWARE_ROOT)/build
include $(FIRMWARE_ROOT)/mk/gtest.mk

CXX       ?= g++
CXXFLAGS  ?= -std=c++14 -Wall -Wextra
INC       := -I$(FIRMWARE_ROOT)
SIM_ARGS  ?=

define require_module
$(if $(MODULE),,$(error MODULE is required. Example: make test MODULE=hdlc))
$(if $(filter $(MODULE),$(MODULES)),,$(error Unknown MODULE='$(MODULE)'. Known: $(MODULES)))
endef

SRCS     = $($(MODULE)_SRCS)
TEST_BIN = $($(MODULE)_TEST)
BIN_DIR  = $(BUILD_DIR)/$(MODULE)
BIN      = $(BIN_DIR)/$(TEST_BIN)

.PHONY: require_module_ok build-one test-one gtest

require_module_ok:
	@$(call require_module)

$(BIN): $(SRCS) $(GTEST_LIB_FILES) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(INC) $(GTEST_INC) -o $@ $(SRCS) $(GTEST_LDFLAGS)

$(BIN_DIR):
	mkdir -p $@

build-one: require_module_ok $(BIN)

test-one: require_module_ok $(BIN)
	@echo "== test $(MODULE) =="
	@$(BIN) $(SIM_ARGS)

test:
	@set -e; \
	for m in $(MODULES); do \
		$(MAKE) -C $(FIRMWARE_ROOT) test-one MODULE=$$m; \
	done

build:
	@set -e; \
	for m in $(MODULES); do \
		$(MAKE) -C $(FIRMWARE_ROOT) build-one MODULE=$$m; \
	done

clean:
	rm -rf $(BUILD_DIR)
