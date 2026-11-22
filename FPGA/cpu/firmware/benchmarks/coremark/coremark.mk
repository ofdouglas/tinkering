COREMARK_SRC := $(THIRD_PARTY_DIR)/coremark
COREMARK_PORT := benchmarks/coremark
COREMARK_NAMES := core_main core_list_join core_matrix core_state core_util
COREMARK_DIR := $(BUILD_DIR)/$(COREMARK_PORT)
COREMARK_OBJS := $(addprefix $(COREMARK_DIR)/,$(addsuffix .o,$(COREMARK_NAMES) ee_printf))
COREMARK_FLAGS := $(ARCH) -O3 -ffreestanding -nostdlib
COREMARK_CC = $(PREFIX)gcc $(COREMARK_FLAGS) -I$(COREMARK_SRC) -I$(COREMARK_PORT) \
	-DCOMPILER_FLAGS='"$(COREMARK_FLAGS)"' -MMD -MP

$(COREMARK_DIR)/%.o: $(COREMARK_SRC)/%.c $(COREMARK_PORT)/coremark.mk Makefile
	@mkdir -p $(dir $@)
	$(COREMARK_CC) -c $< -o $@

$(COREMARK_DIR)/ee_printf.c: $(COREMARK_SRC)/barebones/ee_printf.c $(COREMARK_PORT)/ee_printf.patch
	@mkdir -p $(dir $@)
	patch --silent -o $@.tmp $< < $(COREMARK_PORT)/ee_printf.patch
	mv $@.tmp $@

$(COREMARK_DIR)/ee_printf.o: $(COREMARK_DIR)/ee_printf.c $(COREMARK_PORT)/coremark.mk Makefile
	@mkdir -p $(dir $@)
	$(COREMARK_CC) -c $< -o $@

$(COREMARK_DIR)/coremark_validation.o: COREMARK_SEED_FLAG = -DVALIDATION_RUN=1
$(COREMARK_DIR)/coremark.o $(COREMARK_DIR)/coremark_validation.o: $(COREMARK_PORT)/core_portme.c $(COREMARK_PORT)/coremark.mk Makefile
	@mkdir -p $(dir $@)
	$(COREMARK_CC) $(COREMARK_SEED_FLAG) -c $< -o $@

$(BUILD_DIR)/coremark.elf $(BUILD_DIR)/coremark_validation.elf: $(BUILD_DIR)/%.elf: $(COREMARK_DIR)/%.o $(COREMARK_OBJS) $(BUILD_DIR)/$(COREMARK_PORT)/board.o $(SHARED_OBJS) $(APP_LDSCRIPT)
	$(PREFIX)g++ $(LDFLAGS) -T $(APP_LDSCRIPT) -o $@ $(filter %.o,$^) -lgcc

-include $(wildcard $(COREMARK_DIR)/*.d)

.PHONY: coremark
coremark:
	$(MAKE) board-test BOARD_TEST=coremark_validation
	@echo "CoreMark performance run (score in measured iterations/sec)"
	$(MAKE) board-test BOARD_TEST=coremark
