ifeq ($(ARGS),)
	ARGS=./example/verdana.bmp verdana --outdir ./example
endif
default: run

clean:
	rm -f ./bin/test.elf ./bin/font-parse

build: clean bin ../lib ../lib/libbmpparse.so
	gcc -Wall -Wextra `pkg-config --cflags --libs sdl2` -O3 -I../lib/include -L../lib -lbmpparse ./src/main.c -o ./bin/font-parse

run: clean build
	LD_LIBRARY_PATH=../lib ./bin/font-parse $(ARGS)

debug:
	rm -f test.elf
	gcc -Wall -Wextra -g `pkg-config --cflags --libs sdl2` -I../lib/include -L../lib -lbmpparse ./src/main.c -o ./bin/test.elf
	LD_LIBRARY_PATH=../lib gdb	./bin/test.elf

bin:
	mkdir -p bin

../lib: ../lib/include
	mkdir -p ../lib

../lib/include:
	mkdir -p ../lib/include
	cp -t ../lib/include ../bmpparse/include/*.h

../lib/libbmpparse.so:
	./make_bmpparse.sh
	cp ../bmpparse/bin/libbmpparse.so ../lib
