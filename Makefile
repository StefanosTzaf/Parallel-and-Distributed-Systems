CC = gcc
CFLAGS = -Wall -g -pthread

OBJ1 = 1_1.o 
OBJ2 = 1_2.o
OBJ3 = 1_3.o
OBJ4 = 1_4.o
OBJ5 = 1_5.o
OBJ5_2 = 1_5_mutex_condvar.o
OBJ5_3 = 1_5_busy_waiting.o

TARGET1 = 1_1
TARGET2 = 1_2
TARGET3 = 1_3
TARGET4 = 1_4
TARGET5 = 1_5
TARGET5_2 = 1_5_mutex_condvar
TARGET5_3 = 1_5_busy_waiting

all: $(TARGET1) $(TARGET2) $(TARGET3) $(TARGET5) $(TARGET4) $(TARGET5_2) $(TARGET5_3)

$(TARGET1): 1_1.o
	$(CC) $(CFLAGS) -o $(TARGET1) $(OBJ1)

$(TARGET2): 1_2.o
	$(CC) $(CFLAGS) -o $(TARGET2) $(OBJ2)

$(TARGET3): 1_3.o
	$(CC) $(CFLAGS) -o $(TARGET3) $(OBJ3)

$(TARGET4): 1_4.o
	$(CC) $(CFLAGS) -o $(TARGET4) $(OBJ4)

$(TARGET5): 1_5.o
	$(CC) $(CFLAGS) -o $(TARGET5) $(OBJ5)

$(TARGET5_2): 1_5_mutex_condvar.o
	$(CC) $(CFLAGS) -o $(TARGET5_2) $(OBJ5_2)

$(TARGET5_3): 1_5_busy_waiting.o
	$(CC) $(CFLAGS) -o $(TARGET5_3) $(OBJ5_3)

# Run targets with example arguments

#2 threads, 20 iterations, mutex/rwlock/atomic
run2:
	./$(TARGET2) 4 10000 1   
	./$(TARGET2) 4 10000 2
	./$(TARGET2) 4 10000 3

# size of array
run3:
	./$(TARGET3) 1000000

# number of threads, number of iterations
run5:
	./$(TARGET5) 4 1000000
	./$(TARGET5_2) 4 1000000
	./$(TARGET5_3) 4 1000000

clean:
	rm -f $(TARGET1) $(OBJ1) $(TARGET2) $(OBJ2) $(TARGET3) $(OBJ3) $(TARGET5) $(OBJ5) $(TARGET5_2) $(OBJ5_2)
	rm -f $(TARGET4) $(OBJ4) $(TARGET5_3) $(OBJ5_3)