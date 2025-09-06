
CC=gcc
CFLAGS=-O2 -Wall -Wextra -pthread
TARGET=library_sim

SRC=src/main.c

.PHONY: all build run clean

all: build

build: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $(SRC)

run: build
	./$(TARGET)

clean:
	rm -f $(TARGET)
