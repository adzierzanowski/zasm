DEBUG = 1
SRC = src
BUILD = build
TARGET = zasm

CFLAGS = -Wall -Wpedantic

ifeq ($(DEBUG), 1)
CFLAGS += -O0 -g -DDEBUG
else
	CFLAGS += -O3
endif

OBJS = main.o util.o tokenizer.o emitter.o config.o expressions.o opcodes.o

.PHONY: all
all: $(TARGET)

$(TARGET): $(addprefix $(BUILD)/, $(OBJS))
	$(CC) $(CFLAGS) $^ -o $@

.PHONY: $(SRC)/%.c
$(SRC)/%.c: $(SRC)/%.h

$(BUILD)/%.o: $(SRC)/%.c Makefile
	@- mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

leaks:
	leaks --atExit -- ./zasm -f test/test.s -vv

clean:
	- rm -rf $(BUILD)
	- rm $(TARGET)
