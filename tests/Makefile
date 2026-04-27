# =============================================================================
# GaggiaController — Host-side TDD Makefile
# Build and run Unity unit tests on Windows (MinGW-w64 gcc)
# Usage: make -f tests/Makefile all
# =============================================================================

CC      = gcc
OUTDIR  = tests/out

# ---- Common flags -----------------------------------------------------------
CFLAGS  = -Wall -Wextra -Wno-unused-parameter -Wno-old-style-declaration -Wno-missing-field-initializers -std=c99
CFLAGS += -I tests/unity
CFLAGS += -I ble_espresso_app/components/Utilities
CFLAGS += -I ble_espresso_app/components/Utilities/include
LDFLAGS = -lm

# ---- Phase 2 extra flags (added per-target below) ---------------------------
CFLAGS2  = $(CFLAGS)
CFLAGS2 += -DTEST -DHOST
CFLAGS2 += -Wno-unused-variable
CFLAGS2 += -I tests/stubs
CFLAGS2 += -I tests/fff
CFLAGS2 += -I ble_espresso_app/components/Application
CFLAGS2 += -I ble_espresso_app/components/Peripherals/include
CFLAGS2 += -I ble_espresso_app/components/BLE/include

# ---- Source paths -----------------------------------------------------------
SRC_UNITY       = tests/unity/unity.c
SRC_NUMBERS     = ble_espresso_app/components/Utilities/x04_Numbers.c
SRC_FILTERS     = ble_espresso_app/components/Utilities/x201_DigitalFiltersAlgorithm.c
SRC_PID         = ble_espresso_app/components/Utilities/x205_PID_Block.c
SRC_TEMP_CTRL   = ble_espresso_app/components/Application/tempController.c
SRC_PUMP_CTRL   = ble_espresso_app/components/Application/PumpController.c
SRC_BLE_SVC     = ble_espresso_app/components/Application/BLEspressoServices.c
SRC_STORAGE     = ble_espresso_app/components/Application/StorageController.c
SRC_VALIDATOR   = ble_espresso_app/components/Application/ProfileValidator.c

# =============================================================================
# Phase 1 targets — pure algorithm, no stubs needed
# =============================================================================

.PHONY: all phase1 phase2 phase2_3 clean

all: phase1 phase2 phase2_3

phase1: $(OUTDIR)/test_numbers.exe $(OUTDIR)/test_digital_filters.exe $(OUTDIR)/test_pid_block.exe
	@echo.
	@echo === Running Phase 1 Tests ===
	-$(OUTDIR)/test_numbers.exe
	-$(OUTDIR)/test_digital_filters.exe
	-$(OUTDIR)/test_pid_block.exe
	@echo.
	@echo === Phase 1 Complete ===

$(OUTDIR)/test_numbers.exe: tests/test_numbers.c $(SRC_NUMBERS) $(SRC_UNITY)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(OUTDIR)/test_digital_filters.exe: tests/test_digital_filters.c $(SRC_FILTERS) $(SRC_UNITY)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(OUTDIR)/test_pid_block.exe: tests/test_pid_block.c $(SRC_PID) $(SRC_FILTERS) $(SRC_NUMBERS) $(SRC_UNITY)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# =============================================================================
# Phase 2 targets — application logic with FFF fakes and nRF SDK stubs
# =============================================================================

phase2: $(OUTDIR)/test_temp_controller.exe \
        $(OUTDIR)/test_pump_controller.exe \
        $(OUTDIR)/test_blespresso_services.exe \
        $(OUTDIR)/test_storage_controller.exe
	@echo.
	@echo === Running Phase 2 Tests ===
	-$(OUTDIR)/test_temp_controller.exe
	-$(OUTDIR)/test_pump_controller.exe
	-$(OUTDIR)/test_blespresso_services.exe
	-$(OUTDIR)/test_storage_controller.exe
	@echo.
	@echo === Phase 2 Complete ===

# tempController: compile with real PID/filter/numbers libs so the full signal
# path is exercised under test.  Hardware drivers are replaced by FFF fakes
# declared in tests/fakes.h (hardware layer only).
$(OUTDIR)/test_temp_controller.exe: \
        tests/test_temp_controller.c \
        $(SRC_TEMP_CTRL) \
        $(SRC_PID) $(SRC_FILTERS) $(SRC_NUMBERS) \
        $(SRC_UNITY)
	$(CC) $(CFLAGS2) $^ -o $@ $(LDFLAGS)

# PumpController: state machine and slope calculations only.
$(OUTDIR)/test_pump_controller.exe: \
        tests/test_pump_controller.c \
        $(SRC_PUMP_CTRL) \
        $(SRC_UNITY)
	$(CC) $(CFLAGS2) $^ -o $@ $(LDFLAGS)

# BLEspressoServices: test the three service state machines in isolation.
# tempController and all hardware drivers are faked.
$(OUTDIR)/test_blespresso_services.exe: \
        tests/test_blespresso_services.c \
        $(SRC_BLE_SVC) \
        $(SRC_UNITY)
	$(CC) $(CFLAGS2) $^ -o $@ $(LDFLAGS)

# StorageController: NVM serialisation / deserialisation logic.
# spi_NVMemoryRead / spi_NVMemoryWritePage are replaced by custom-fake callbacks.
# Includes ProfileValidator so the M3 fix (NVM range clamp) is exercised.
$(OUTDIR)/test_storage_controller.exe: \
        tests/test_storage_controller.c \
        $(SRC_STORAGE) \
        $(SRC_VALIDATOR) \
        $(SRC_UNITY)
	$(CC) $(CFLAGS2) $^ -o $@ $(LDFLAGS)

# =============================================================================
# Phase 2/3 targets — remaining application issues (H2, H5, M5, M3 fix)
# =============================================================================

phase2_3: $(OUTDIR)/test_profile_validator.exe
	@echo.
	@echo === Running Phase 2/3 Tests ===
	-$(OUTDIR)/test_profile_validator.exe
	@echo.
	@echo === Phase 2/3 Complete ===

$(OUTDIR)/test_profile_validator.exe: \
        tests/test_profile_validator.c \
        $(SRC_VALIDATOR) \
        $(SRC_UNITY)
	$(CC) $(CFLAGS2) $^ -o $@ $(LDFLAGS)

# =============================================================================
# Utility
# =============================================================================

clean:
	@if exist $(OUTDIR)\*.exe del /Q $(OUTDIR)\*.exe
	@echo Cleaned.

