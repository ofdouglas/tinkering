# Host C++ unit tests for protocol and algorithm code under Firmware/.

FIRMWARE_ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
MODULE        ?=

include mk/host.mk

.PHONY: help list

.DEFAULT_GOAL := help

help:
	@echo "Firmware host tests"
	@echo ""
	@echo "  make list                  Modules with <name>_test.cpp"
	@echo "  make build                 Build all test binaries"
	@echo "  make build-one MODULE=<n>  Build one module"
	@echo "  make test                  Run all tests"
	@echo "  make test-one MODULE=<n>   Build and run one test"
	@echo "  make clean                 Remove build/ (incl. googletest fetch)"
	@echo ""
	@echo "  SIM_ARGS='--gtest_filter=...'  Pass flags to test binary"
	@echo ""
	@echo "Modules: $(MODULES)"

list:
	@echo $(MODULES)
