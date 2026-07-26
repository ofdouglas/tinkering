# Common build rules (expects TARGET, BUILD_DIR, LINKER_SCRIPT, sources from project).

BUILD_DIR ?= build

C_OBJS   = $(addprefix $(BUILD_DIR)/,$(C_SOURCES:.c=.o))
CPP_OBJS = $(addprefix $(BUILD_DIR)/,$(CPP_SOURCES:.cpp=.o))
ASM_OBJS = $(addprefix $(BUILD_DIR)/,$(ASM_SOURCES:.s=.o))
OBJECTS  = $(C_OBJS) $(CPP_OBJS) $(ASM_OBJS)
DEPS     = $(OBJECTS:.o=.d)

.PHONY: all clean size

all: $(BUILD_DIR)/$(TARGET).elf size

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) $(LINKER_SCRIPT)
	$(LD) $(OBJECTS) $(LDFLAGS) -o $@

$(BUILD_DIR)/%.o: %.c
	@$(MKDIR) $(dir $@)
	$(CC) -c $(CFLAGS) -MMD -MP -MF $(@:.o=.d) -o $@ $<

$(BUILD_DIR)/%.o: %.cpp
	@$(MKDIR) $(dir $@)
	$(CXX) -c $(CXXFLAGS) -MMD -MP -MF $(@:.o=.d) -o $@ $<

$(BUILD_DIR)/%.o: %.s
	@$(MKDIR) $(dir $@)
	$(AS) -c $(MCU) $(C_DEFS) $(INCLUDES) -x assembler-with-cpp \
		--specs=nano.specs -MMD -MP -MF $(@:.o=.d) -o $@ $<

size: $(BUILD_DIR)/$(TARGET).elf
	$(SZ) --format=berkeley $<

clean:
	rm -rf $(BUILD_DIR)

-include $(DEPS)
