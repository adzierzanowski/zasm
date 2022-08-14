DEBUG = 0
SRC = src
BUILD = build
TARGET = zasm

CFLAGS = -Wall -Wpedantic \
-Wno-gnu-binary-literal

ifeq ($(DEBUG), 1)
CFLAGS += -Og -g -DDEBUG
else
	CFLAGS += -O3
endif

OBJS = main.o util.o tokenizer.o opcodes.o emitter.o config.o

.PHONY: all
all: $(TARGET) Makefile

$(TARGET): $(addprefix $(BUILD)/, $(OBJS))
	$(CC) $(CFLAGS) $^ -o $@

.PHONY: $(SRC)/%.c
$(SRC)/%.c: $(SRC)/%.h

$(BUILD)/%.o: $(SRC)/%.c
	@- mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	- rm -rf $(BUILD)
	- rm $(TARGET)
