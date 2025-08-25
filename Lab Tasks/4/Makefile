#Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11

#Target executable
TARGET = myprogram

#Object files
OBJS = main.o a.o b.o

#Default target
all: $(TARGET)

#Link object files into executable
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)

#Compile each .c into .o
main.o: main.c
	$(CC) $(CFLAGS) -c main.c

a.o: a.c
	$(CC) $(CFLAGS) -c a.c

b.o: b.c
	$(CC) $(CFLAGS) -c b.c

#Remove compiled files
clean:
	rm -f $(OBJS) $(TARGET)

#
Build and run
run: $(TARGET)
	./$(TARGET)
