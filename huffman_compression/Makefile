SRC=./src
INC=./include
BIN=./bin

OBJS=$(shell find ./src -iname *.c -type f | sed 's-\./src-\./bin-g' | sed 's/\.c/\.o/g')
CFLAGS=-Wall -Werror -Wextra -I$(INC) -g -Wno-unused-function $(MACROS)
LDFLAGS=-Wall -Werror -Wextra -g
CC=clang

TARGET=huffman.elf

default: clean build run

leaktest: clean build
	valgrind --leak-check=full ./bin/$(TARGET)

run: clean build
	./bin/$(TARGET)


build: clean $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $^ $(LDFLAGS) -o ./bin/$@

$(OBJS): $(BIN)/%.o : $(SRC)/%.c
	$(CC) -c $< $(CFLAGS) -o $@

clean:
	rm -f ./bin/*.?*

