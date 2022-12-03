CC=cc
CFLAGS=-std=c99 -Wall -Wextra -Werror

.PHONY: all
all: bta

bta: main.c
	$(CC) $(CFLAGS) -o $@ $^

.PHONY: run
run: bta
	 @./bta $(FILENAME)

.PHONY: clean
clean:
	rm -f ./bta
