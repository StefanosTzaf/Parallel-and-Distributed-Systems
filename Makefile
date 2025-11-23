CC = gcc
CFLAGS = -Wall -g -pthread

OBJ1 = 1_1.o 
OBJ2 = 1_2.o

TARGET1 = 1_1
TARGET2 = 1_2

all: $(TARGET1) $(TARGET2)

$(TARGET1): 1_1.o
	$(CC) $(CFLAGS) -o $(TARGET1) $(OBJ1)

$(TARGET2): 1_2.o
	$(CC) $(CFLAGS) -o $(TARGET2) $(OBJ2)

# Run targets with example arguments
#2 threads, 20 iterations, mutex/rwlock/atomic
run2:
	./$(TARGET2) 2 20 1   
	./$(TARGET2) 2 20 2
	./$(TARGET2) 2 20 3


clean:
	rm -f $(TARGET1) $(OBJ1) $(TARGET2) $(OBJ2)
