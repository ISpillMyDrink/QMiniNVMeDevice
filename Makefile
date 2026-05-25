CC ?= cc
CFLAGS ?= -O2 -Wall -Wextra -Wpedantic -std=c11
AR ?= ar
ARFLAGS ?= rcs

MININVME_INCLUDE ?=

SRC_DIR := src
EXAMPLES_DIR := examples
BUILD_DIR := build
LIB_DIR := $(BUILD_DIR)/lib
OBJ_DIR := $(BUILD_DIR)/obj
BIN_DIR := $(BUILD_DIR)/bin

LIB_NAME := libmininvme_user.a
LIB_PATH := $(LIB_DIR)/$(LIB_NAME)

INCLUDES := -I$(SRC_DIR)
ifneq ($(strip $(MININVME_INCLUDE)),)
INCLUDES += -I$(MININVME_INCLUDE)
endif

LIB_SRCS := $(SRC_DIR)/mininvme_user.c
LIB_OBJS := $(OBJ_DIR)/mininvme_user.o
EXAMPLE_SRC := $(EXAMPLES_DIR)/mininvme_cli_example.c
EXAMPLE_BIN := $(BIN_DIR)/mininvme_cli_example

.PHONY: all clean lib example

all: lib example

lib: $(LIB_PATH)

example: $(EXAMPLE_BIN)

$(OBJ_DIR) $(LIB_DIR) $(BIN_DIR):
	mkdir -p $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(LIB_PATH): $(LIB_OBJS) | $(LIB_DIR)
	$(AR) $(ARFLAGS) $@ $^

$(EXAMPLE_BIN): $(EXAMPLE_SRC) $(LIB_PATH) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -lmininvme_user -o $@

clean:
	rm -rf $(BUILD_DIR)
