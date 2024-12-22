lib: clean ./bin
	rm -f ./bin/libbmpparse.so
	gcc -std=c99 -O3 -Wall -Wextra -fpic -shared -o ./bin/libbmpparse.so ./src/bmp_parse.c

clean: ./bin
	rm -f ./bin/libbmpparse.so
