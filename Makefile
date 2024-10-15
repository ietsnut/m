SRC = m.c
TARGET = m.exe

ifeq ($(OS),Windows_NT)
    CLEAN = del /F /Q $(TARGET) $(shell dir /b massif.out.*) $(shell dir /b *.su)
else
    CLEAN = rm -f $(TARGET) $(wildcard massif.out.*) $(SRC:.c=.su)
endif

all: build run clean

build: $(SRC)
	cosmocc -o $(TARGET) $(SRC)

run:
	./$(TARGET)

clean:where cos
	$(CLEAN)

report: build
	valgrind --tool=massif ./$(TARGET)

.PHONY: run clean report build