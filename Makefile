CFLAGS = -g -fstack-usage -static -Iinclude -Llib -Wall -Wextra -pedantic 
SRC = ./main.c

ifeq ($(OS),Windows_NT)
    TARGET = main.exe
    RM = del /F /Q
else
    TARGET = main
    RM = rm -f
endif

main: build run

build: $(SRC)
	gcc $(CFLAGS) -o $(TARGET) $(SRC)

run:
	./$(TARGET)

clean:
	$(RM) $(TARGET)

report:
	valgrind --tool=massif ./$(TARGET)