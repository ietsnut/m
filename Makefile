FILE = edit

SRC = $(FILE).c
TARGET = $(FILE).exe

ifeq ($(OS),Windows_NT)
    CLEAN = del /F /Q $(TARGET) $(shell dir /b massif.out.*) $(shell dir /b *.su)
else
    CLEAN = rm -f $(TARGET) $(wildcard massif.out.*) $(SRC:.c=.su)
endif

all: build run clean

build: $(SRC)
	$(CC) -o $(TARGET) $(SRC) -Wall -Wextra -pedantic -std=c99

run:
	./$(TARGET)


clean:
	$(CLEAN)

report: build
	valgrind --tool=massif ./$(TARGET)

.PHONY: run clean report build
