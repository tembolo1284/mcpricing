#==============================================================================
#  mcoptions - Monte Carlo Options Pricing Library
#  Makefile v2.5.0
#==============================================================================
#
#  A production-grade Monte Carlo options pricing library in pure C.
#
#  Usage:
#    make                Build release libraries
#    make BUILD=debug    Build with debug symbols and sanitizers
#    make run-tests      Build and run all 140 tests
#    make install        Install to system (default: /usr/local)
#    make clean          Remove build artifacts
#    make help           Show detailed help
#
#==============================================================================

.PHONY: all lib shared static test run-tests clean install uninstall help info

#------------------------------------------------------------------------------
# Configuration
#------------------------------------------------------------------------------

CC        := gcc
AR        := ar
INSTALL   := install

BUILD     ?= release
PREFIX    ?= /usr/local
LIBDIR    := $(PREFIX)/lib
INCDIR    := $(PREFIX)/include

LIB_NAME  := mcoptions
VERSION   := 2.5.0

#------------------------------------------------------------------------------
# Directories
#------------------------------------------------------------------------------

SRC_DIR   := src
INC_DIR   := include
TEST_DIR  := tests
BUILD_DIR := build
OBJ_DIR   := $(BUILD_DIR)/obj

#------------------------------------------------------------------------------
# Compiler Flags
#------------------------------------------------------------------------------

CFLAGS_BASE := -std=c11 -Wall -Wextra -Wpedantic -Wno-unused-function
CFLAGS_BASE += -fPIC -fvisibility=hidden

CFLAGS_RELEASE := -O3 -DNDEBUG -march=native -flto
CFLAGS_DEBUG   := -g3 -O0 -DDEBUG -fsanitize=address,undefined -fno-omit-frame-pointer

ifeq ($(BUILD),debug)
    CFLAGS := $(CFLAGS_BASE) $(CFLAGS_DEBUG)
    LDFLAGS_EXTRA := -fsanitize=address,undefined
else
    CFLAGS := $(CFLAGS_BASE) $(CFLAGS_RELEASE)
    LDFLAGS_EXTRA := -flto
endif

LDFLAGS := -lm -lpthread $(LDFLAGS_EXTRA)
INCLUDES := -I$(INC_DIR)

#------------------------------------------------------------------------------
# Sources
#------------------------------------------------------------------------------

# Core
SRCS := $(SRC_DIR)/rng.c \
        $(SRC_DIR)/allocator.c \
        $(SRC_DIR)/context.c \
        $(SRC_DIR)/version.c

# Models
SRCS += $(SRC_DIR)/models/gbm.c \
        $(SRC_DIR)/models/sabr.c \
        $(SRC_DIR)/models/sabr_pricing.c \
        $(SRC_DIR)/models/black76.c \
        $(SRC_DIR)/models/heston.c \
        $(SRC_DIR)/models/merton_jump.c

# Instruments
SRCS += $(SRC_DIR)/instruments/european.c \
        $(SRC_DIR)/instruments/american.c \
        $(SRC_DIR)/instruments/asian.c \
        $(SRC_DIR)/instruments/bermudan.c \
        $(SRC_DIR)/instruments/barrier.c \
        $(SRC_DIR)/instruments/lookback.c \
        $(SRC_DIR)/instruments/digital.c

# Methods
SRCS += $(SRC_DIR)/methods/thread_pool.c \
        $(SRC_DIR)/methods/lsm.c \
        $(SRC_DIR)/methods/sobol.c

# Variance Reduction
SRCS += $(SRC_DIR)/variance_reduction/control_variates.c

OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

#------------------------------------------------------------------------------
# Tests
#------------------------------------------------------------------------------

TEST_SRCS := $(TEST_DIR)/test_rng.c \
             $(TEST_DIR)/test_context.c \
             $(TEST_DIR)/test_european.c \
             $(TEST_DIR)/test_american.c \
             $(TEST_DIR)/test_asian.c \
             $(TEST_DIR)/test_bermudan.c \
             $(TEST_DIR)/test_sabr.c \
             $(TEST_DIR)/test_black76.c \
             $(TEST_DIR)/test_control_variates.c \
             $(TEST_DIR)/test_heston.c \
             $(TEST_DIR)/test_merton.c \
             $(TEST_DIR)/test_barrier.c \
             $(TEST_DIR)/test_lookback.c \
             $(TEST_DIR)/test_digital.c

TEST_BINS := $(patsubst $(TEST_DIR)/%.c,$(BUILD_DIR)/%,$(TEST_SRCS))
UNITY_OBJ := $(OBJ_DIR)/unity.o

#------------------------------------------------------------------------------
# Outputs
#------------------------------------------------------------------------------

LIB_SHARED := $(BUILD_DIR)/lib$(LIB_NAME).so
LIB_STATIC := $(BUILD_DIR)/lib$(LIB_NAME).a

#------------------------------------------------------------------------------
# Default Target
#------------------------------------------------------------------------------

all: lib

#------------------------------------------------------------------------------
# Library Targets
#------------------------------------------------------------------------------

lib: $(LIB_SHARED) $(LIB_STATIC)

$(LIB_SHARED): $(OBJS)
	@echo ""
	@echo "╔════════════════════════════════════════════╗"
	@echo "║  Building libmcoptions.so                  ║"
	@echo "╚════════════════════════════════════════════╝"
	@mkdir -p $(BUILD_DIR)
	@$(CC) -shared -Wl,-soname,lib$(LIB_NAME).so -o $@ $^ $(LDFLAGS)
	@echo "  ✓ Created: $@"

$(LIB_STATIC): $(OBJS)
	@echo ""
	@echo "╔════════════════════════════════════════════╗"
	@echo "║  Building libmcoptions.a                   ║"
	@echo "╚════════════════════════════════════════════╝"
	@mkdir -p $(BUILD_DIR)
	@$(AR) rcs $@ $^
	@echo "  ✓ Created: $@"

#------------------------------------------------------------------------------
# Object Files
#------------------------------------------------------------------------------

# Create all necessary subdirectories
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "  CC  $<"
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $(INCLUDES) -DMCO_BUILD_SHARED -c $< -o $@

# Ensure build directories exist
$(OBJS): | $(OBJ_DIR)

$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)/models
	@mkdir -p $(OBJ_DIR)/instruments
	@mkdir -p $(OBJ_DIR)/methods
	@mkdir -p $(OBJ_DIR)/variance_reduction

#------------------------------------------------------------------------------
# Tests
#------------------------------------------------------------------------------

test: $(TEST_BINS)

$(UNITY_OBJ): $(TEST_DIR)/unity/unity.c
	@echo "  CC  $<"
	@mkdir -p $(OBJ_DIR)
	@$(CC) $(CFLAGS_BASE) -I$(TEST_DIR) -c $< -o $@

$(BUILD_DIR)/test_%: $(TEST_DIR)/test_%.c $(LIB_STATIC) $(UNITY_OBJ)
	@echo "  CC  $<"
	@mkdir -p $(BUILD_DIR)
	@$(CC) $(CFLAGS) $(INCLUDES) -I$(TEST_DIR) $< $(UNITY_OBJ) $(LIB_STATIC) -o $@ $(LDFLAGS)

run-tests: test
	@echo ""
	@echo "╔════════════════════════════════════════════════════════════════╗"
	@echo "║  mcoptions v$(VERSION) - Running Tests                            ║"
	@echo "╚════════════════════════════════════════════════════════════════╝"
	@echo ""
	@total_tests=0; failed_suites=0; \
	for t in $(TEST_BINS); do \
		if $$t; then :; else failed_suites=$$((failed_suites + 1)); fi; \
		echo ""; \
	done; \
	echo "════════════════════════════════════════════════════════════════════"; \
	if [ $$failed_suites -eq 0 ]; then \
		echo "  ✓ ALL 140 TESTS PASSED (14 test suites)"; \
	else \
		echo "  ✗ $$failed_suites test suite(s) failed"; \
		exit 1; \
	fi; \
	echo "════════════════════════════════════════════════════════════════════"

#------------------------------------------------------------------------------
# Installation
#------------------------------------------------------------------------------

install: lib
	@echo ""
	@echo "Installing mcoptions $(VERSION) to $(PREFIX)..."
	@$(INSTALL) -d $(DESTDIR)$(LIBDIR)
	@$(INSTALL) -d $(DESTDIR)$(INCDIR)
	@$(INSTALL) -m 644 $(LIB_STATIC) $(DESTDIR)$(LIBDIR)/
	@$(INSTALL) -m 755 $(LIB_SHARED) $(DESTDIR)$(LIBDIR)/
	@$(INSTALL) -m 644 $(INC_DIR)/mcoptions.h $(DESTDIR)$(INCDIR)/
	@echo "  ✓ Installed libraries to $(LIBDIR)"
	@echo "  ✓ Installed header to $(INCDIR)"
	@echo ""
	@echo "Run 'sudo ldconfig' to update the library cache."

uninstall:
	@echo "Uninstalling mcoptions from $(PREFIX)..."
	@rm -f $(DESTDIR)$(LIBDIR)/lib$(LIB_NAME).a
	@rm -f $(DESTDIR)$(LIBDIR)/lib$(LIB_NAME).so
	@rm -f $(DESTDIR)$(INCDIR)/mcoptions.h
	@echo "  ✓ Done"

#------------------------------------------------------------------------------
# Clean
#------------------------------------------------------------------------------

clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR)
	@echo "  ✓ Removed $(BUILD_DIR)/"

#------------------------------------------------------------------------------
# Info
#------------------------------------------------------------------------------

info:
	@echo ""
	@echo "mcoptions $(VERSION)"
	@echo ""
	@echo "  Compiler:     $(CC)"
	@echo "  Build:        $(BUILD)"
	@echo "  Prefix:       $(PREFIX)"
	@echo "  Sources:      $(words $(SRCS)) files"
	@echo "  Test suites:  $(words $(TEST_SRCS))"
	@echo ""

#------------------------------------------------------------------------------
# Help
#------------------------------------------------------------------------------

help:
	@echo ""
	@echo "╔══════════════════════════════════════════════════════════════════╗"
	@echo "║  mcoptions - Monte Carlo Options Pricing Library                 ║"
	@echo "║  Version $(VERSION)                                                   ║"
	@echo "╚══════════════════════════════════════════════════════════════════╝"
	@echo ""
	@echo "  A production-grade Monte Carlo options pricing library in pure C."
	@echo ""
	@echo "  Models:      GBM, SABR, Heston, Merton Jump, Black-76"
	@echo "  Instruments: European, American, Asian, Bermudan, Barrier,"
	@echo "               Lookback, Digital"
	@echo "  Methods:     Pseudo-random, Quasi-random (Sobol), LSM,"
	@echo "               Antithetic, Control Variates, Multithreading"
	@echo ""
	@echo "Usage:"
	@echo "  make                  Build optimized libraries"
	@echo "  make BUILD=debug      Build with sanitizers"
	@echo "  make run-tests        Run all 140 tests"
	@echo "  make install          Install to $(PREFIX)"
	@echo "  make clean            Remove build artifacts"
	@echo "  make info             Show configuration"
	@echo ""
	@echo "Options:"
	@echo "  BUILD=debug|release   Build mode (default: release)"
	@echo "  PREFIX=/path          Install prefix (default: /usr/local)"
	@echo ""
	@echo "Output:"
	@echo "  build/libmcoptions.so   Shared library"
	@echo "  build/libmcoptions.a    Static library"
	@echo ""
