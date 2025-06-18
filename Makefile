CC = gcc
CFLAGS = -O2 -Wall

benchmark: src/benchmark.c
	$(CC) $(CFLAGS) -o benchmark src/benchmark.c

clean:
	rm -f benchmark result.csv

.PHONY: clean
