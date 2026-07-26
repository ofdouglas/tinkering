# Google Test: fetch on first build, compile static libs into $(BUILD_DIR)/gtest.

GTEST_TAG     ?= v1.15.2
GTEST_SRC_DIR := $(BUILD_DIR)/third_party/googletest
GTEST_DIR     := $(BUILD_DIR)/gtest
GTEST_INC     := -isystem $(GTEST_SRC_DIR)/googletest/include
GTEST_CPPFLAGS := -I$(GTEST_SRC_DIR)/googletest

GTEST_LIB     := $(GTEST_DIR)/libgtest.a
GTEST_MAIN_LIB := $(GTEST_DIR)/libgtest_main.a
GTEST_LIB_FILES := $(GTEST_LIB) $(GTEST_MAIN_LIB)
GTEST_LDFLAGS := $(GTEST_LIB_FILES) -pthread

$(GTEST_SRC_DIR)/.fetched:
	@mkdir -p $(dir $@)
	git clone --depth 1 --branch $(GTEST_TAG) \
		https://github.com/google/googletest.git $(GTEST_SRC_DIR)
	@touch $@

$(GTEST_DIR):
	mkdir -p $@

$(GTEST_LIB): $(GTEST_SRC_DIR)/.fetched | $(GTEST_DIR)
	$(CXX) $(CXXFLAGS) $(GTEST_CPPFLAGS) $(GTEST_INC) -c \
		$(GTEST_SRC_DIR)/googletest/src/gtest-all.cc \
		-o $(GTEST_DIR)/gtest-all.o
	ar rcs $@ $(GTEST_DIR)/gtest-all.o

$(GTEST_MAIN_LIB): $(GTEST_SRC_DIR)/.fetched | $(GTEST_DIR)
	$(CXX) $(CXXFLAGS) $(GTEST_CPPFLAGS) $(GTEST_INC) -c \
		$(GTEST_SRC_DIR)/googletest/src/gtest_main.cc \
		-o $(GTEST_DIR)/gtest_main.o
	ar rcs $@ $(GTEST_DIR)/gtest_main.o

.PHONY: gtest
gtest: $(GTEST_LIB_FILES)
