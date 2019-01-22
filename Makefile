CC=gcc
CFLAGS=-g -Wall -std=c99 -O2
LDFLAGS=-g
TARGET=fish cmdline_test

all:$(TARGET)

fish: fish.o cmdline.o
	$(CC) $(LDFLAGS) $^ -o $@
	
cmdline_test: cmdline_test.o cmdline.o
	$(CC) $(LDFLAGS) $^ -o $@
	
cmdline.o: cmdline.c cmdline.h
	$(CC) $(CFLAGS) -c -o $@ $<

fish.o: fish.c cmdline.h
	$(CC) $(CFLAGS) -c -o $@ $<
	
clean:
	rm -f *.o

mrproper: clean
	rm -f $(TARGET)
