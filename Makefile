CFLAGS = -Wall -Wpedantic \
-Wno-gnu-binary-literal \
-DSHOW_EMIT \
-DDEBUG

#-DSHOW_TOKCHARS \
#-DSHOW_NEW_TOKENS \

.PHONY: all
all: zasm

zasm: main.o util.o tokenizer.o opcodes.o emitter.o
	$(CC) $(CFLAGS) $^ -o zasm

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf *.o
	rm zasm
