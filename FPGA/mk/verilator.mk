# Shared Verilator rules. Included from FPGA/Makefile; expects FPGA_ROOT and modules.mk.

ifndef FPGA_ROOT
$(error FPGA_ROOT must be set before including mk/verilator.mk)
endif

include $(FPGA_ROOT)/mk/modules.mk

BUILD_DIR ?= $(FPGA_ROOT)/build/sim
VERILATOR ?= verilator
CURSOR      ?=
GTKWAVE     ?= gtkwave
WAVE_VIEWER ?= cursor

VERILATOR_FLAGS := -sv --timing -Wno-TIMESCALEMOD -Wno-WIDTHEXPAND -Wno-WIDTHTRUNC \
	-Wno-CASEINCOMPLETE -Wno-BLKSEQ -Wno-UNUSEDSIGNAL --trace-fst
VERILATOR_LINT_FLAGS := --timing -Wno-TIMESCALEMOD -Wno-WIDTHEXPAND -Wno-BLKSEQ \
	-Wno-UNUSEDSIGNAL -Wno-WIDTHTRUNC -Wno-CASEINCOMPLETE -Wno-DECLFILENAME -Wno-UNDRIVEN

define require_module
$(if $(MODULE),,$(error MODULE is required. Example: make sim MODULE=crc))
$(if $(filter $(MODULE),$(MODULES)),,$(error Unknown MODULE='$(MODULE)'. Known: $(MODULES)))
endef

TOP      = $($(MODULE)_TOP)
RTL      = $($(MODULE)_RTL)
TB       = $($(MODULE)_TB)
SIM_DIR  = $(BUILD_DIR)/$(MODULE)
SIM_BIN  = $(SIM_DIR)/V$(TOP)
WAVE_FST = $(SIM_DIR)/$(TOP).fst

.PHONY: require_module_ok lint lint-one trace trace_gen open_wave wave

require_module_ok:
	@$(call require_module)

lint:
	@set -e; \
	for m in $(MODULES); do \
		$(MAKE) -f $(FPGA_ROOT)/Makefile lint-one MODULE=$$m; \
	done

lint-one: require_module_ok
	@echo "== lint $(MODULE) =="
	$(VERILATOR) --lint-only -Wall $(VERILATOR_LINT_FLAGS) --top-module $(TOP) $(RTL) $(TB)

$(SIM_BIN): $(RTL) $(TB) | $(SIM_DIR)
	$(VERILATOR) --binary $(VERILATOR_FLAGS) \
		--top-module $(TOP) \
		--Mdir $(SIM_DIR) \
		$(RTL) $(TB)

$(SIM_DIR):
	mkdir -p $@

sim: require_module_ok $(SIM_BIN)
	$(SIM_BIN) $(if $(filter 1,$(TRACE)),+trace +dumpfile=$(WAVE_FST),) $(SIM_ARGS)

trace_gen: require_module_ok $(SIM_BIN)
	$(SIM_BIN) +trace +dumpfile=$(WAVE_FST) $(SIM_ARGS)
	@echo "Trace: $(WAVE_FST)"

trace: trace_gen

open_wave: require_module_ok
	@test -f '$(WAVE_FST)' || { echo "Missing $(WAVE_FST); run make trace MODULE=$(MODULE) first"; exit 1; }
	@case "$(WAVE_VIEWER)" in \
		cursor) \
			win_path="$$(wslpath -w '$(WAVE_FST)' 2>/dev/null || echo '$(WAVE_FST)')"; \
			if [ -n "$(CURSOR)" ] && [ -x "$(CURSOR)" ]; then \
				"$(CURSOR)" "$$win_path"; \
			elif command -v cursor.exe >/dev/null 2>&1; then \
				cursor.exe "$$win_path"; \
			else \
				win_user="$$(cmd.exe /c echo %USERNAME% 2>/dev/null | tr -d '\r')"; \
				[ -n "$$win_user" ] || win_user="$$(whoami.exe 2>/dev/null | tr -d '\r')"; \
				if [ -z "$$win_user" ]; then \
					case "$$HOME" in /mnt/c/Users/*) win_user="$$(echo "$$HOME" | cut -d/ -f4)";; esac; \
				fi; \
				found=""; \
				for candidate in \
					"/mnt/c/Users/$$win_user/AppData/Local/Programs/cursor/_/Cursor.exe" \
					"/mnt/c/Users/$$win_user/AppData/Local/Programs/Cursor/Cursor.exe" \
					"/mnt/c/Users/$$win_user/AppData/Local/Programs/cursor/Cursor.exe" \
					/mnt/c/Users/*/AppData/Local/Programs/cursor/_/Cursor.exe \
					/mnt/c/Users/*/AppData/Local/Programs/Cursor/Cursor.exe; do \
					if [ -x "$$candidate" ]; then found="$$candidate"; break; fi; \
				done; \
				if [ -n "$$found" ]; then \
					"$$found" "$$win_path"; \
				elif command -v cursor >/dev/null 2>&1; then \
					cursor "$(WAVE_FST)"; \
				else \
					echo "Cursor not found. Set CURSOR=/path/to/Cursor.exe or install Shell Command in Cursor."; \
					echo "FST: $(WAVE_FST)"; \
					exit 1; \
				fi; \
			fi ;; \
		gtkwave) \
			$(GTKWAVE) "$(WAVE_FST)" & ;; \
		*) \
			echo "Unknown WAVE_VIEWER=$(WAVE_VIEWER) (use cursor or gtkwave)"; \
			exit 1 ;; \
	esac

wave: require_module_ok trace_gen open_wave

clean:
	rm -rf $(FPGA_ROOT)/build
