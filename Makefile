CFLAGS = -g -fstack-usage -Wall -Wextra -pedantic 
SRC = ./main.c

ifeq ($(OS),Windows_NT)
    TARGET = main.exe
    CLEAN = del /F /Q $(TARGET) main.exe $(shell dir /b massif.out.*) $(shell dir /b *.su)
else
    TARGET = main
    CLEAN = rm -f $(TARGET) main $(wildcard massif.out.*) $(SRC:.c=.su)
endif

all: build run clean

build: $(SRC)
	gcc $(CFLAGS) -o $(TARGET) $(SRC)

run:
	./$(TARGET)

clean:
	$(CLEAN)

report: build
	valgrind --tool=massif ./$(TARGET)

.PHONY: run clean report build