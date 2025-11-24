CC = gcc
CFLAGS = -Wall -g -pthread

OBJ1 = 1_1.o 
OBJ2 = 1_2.o
OBJ3 = 1_3.o

TARGET1 = 1_1
TARGET2 = 1_2
TARGET3 = 1_3

all: $(TARGET1) $(TARGET2) $(TARGET3)

$(TARGET1): 1_1.o
	$(CC) $(CFLAGS) -o $(TARGET1) $(OBJ1)

$(TARGET2): 1_2.o
	$(CC) $(CFLAGS) -o $(TARGET2) $(OBJ2)

$(TARGET3): 1_3.o
	$(CC) $(CFLAGS) -o $(TARGET3) $(OBJ3)

# Run targets with example arguments
#2 threads, 20 iterations, mutex/rwlock/atomic
run2:
	./$(TARGET2) 4 10000 1   
	./$(TARGET2) 4 10000 2
	./$(TARGET2) 4 10000 3

run3:
	./$(TARGET3) 10000


clean:
	rm -f $(TARGET1) $(OBJ1) $(TARGET2) $(OBJ2) $(TARGET3) $(OBJ3)