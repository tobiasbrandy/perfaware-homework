# tool macros
BASEFLAGS := -std=c17 -Wextra -Wall -pedantic -Wno-unused-parameter -Wno-implicit-fallthrough -Wno-unused-function -Werror=return-type -Wshadow -Wdouble-promotion -Wformat=2 -Wundef -fno-common
CFLAGS := $(BASEFLAGS) -O2
DEVFLAGS := $(BASEFLAGS) -g3 -Og -fsanitize=address,leak,undefined,null,return,signed-integer-overflow,bool,enum,bounds -fno-omit-frame-pointer
DBGFLAGS := -fno-sanitize=address,leak -O0

# path macros
BIN_PATH := bin
OBJ_PATH := obj
SRC_PATH := src
DEV_PATH := dev
DBG_PATH := debug

# compile macros
TARGET_NAME := sim86
ifeq ($(OS),Windows_NT)
	TARGET_NAME := $(addsuffix .exe,$(TARGET_NAME))
endif
TARGET := $(BIN_PATH)/$(TARGET_NAME)
TARGET_DEV := $(DEV_PATH)/$(TARGET_NAME)
TARGET_DEBUG := $(DBG_PATH)/$(TARGET_NAME)

# src files & obj files
SRC := $(foreach x, $(SRC_PATH), $(wildcard $(addprefix $(x)/*,.c)))
OBJ := $(addprefix $(OBJ_PATH)/, $(addsuffix .o, $(notdir $(basename $(SRC)))))
OBJ_DEV := $(addprefix $(DEV_PATH)/, $(addsuffix .o, $(notdir $(basename $(SRC)))))
OBJ_DEBUG := $(addprefix $(DBG_PATH)/, $(addsuffix .o, $(notdir $(basename $(SRC)))))

# clean files list
DISTCLEAN_LIST := $(OBJ) \
				  $(OBJ_DEV) \
                  $(OBJ_DEBUG)
CLEAN_LIST := $(TARGET) \
			  $(TARGET_DEV) \
			  $(TARGET_DEBUG) \
			  $(DISTCLEAN_LIST)

default: makedir all

# non-phony targets
$(TARGET): $(OBJ)
	$(CC) -o $@ $(OBJ) $(CFLAGS)

$(OBJ_PATH)/%.o: $(SRC_PATH)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(DEV_PATH)/%.o: $(SRC_PATH)/%.c
	$(CC) $(DEVFLAGS) -c -o $@ $<

$(DBG_PATH)/%.o: $(SRC_PATH)/%.c
	$(CC) $(DEVFLAGS) $(DBGFLAGS) -c -o $@ $<

$(TARGET_DEV): $(OBJ_DEV)
	$(CC) $(DEVFLAGS) $(OBJ_DEV) -o $@

$(TARGET_DEBUG): $(OBJ_DEBUG)
	$(CC) $(DEVFLAGS) $(DBGFLAGS) $(OBJ_DEBUG) -o $@

.PHONY: all
all: $(TARGET)

.PHONY: makedir
makedir:
	@mkdir -p $(BIN_PATH) $(OBJ_PATH) $(DBG_PATH) $(DEV_PATH)

.PHONY: debug
debug: $(TARGET_DEBUG)

.PHONY: dev
dev: $(TARGET_DEV)

.PHONY: test
test: test_decompile test_run

.PHONY: test_decompile
test_decompile: dev
	./test/decompile/test.sh test/decompile/*.asm

.PHONY: test_run
test_run: dev
	./test/run/test.sh test/run/*.asm

.PHONY: clean
clean:
	@rm -f $(CLEAN_LIST)

.PHONY: distclean
distclean:
	@rm -f $(DISTCLEAN_LIST)
