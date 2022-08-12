CFLAGS = -Wall -Wpedantic -DDEBUG -Wno-gnu-binary-literal


all: main.o util.o tokenizer.o opcodes.o
	$(CC) $(CFLAGS) $^ -o zasm


%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf *.o
	rm zasm
