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
run2:
	./$(TARGET2) 3 3 2

clean:
	rm -f $(TARGET1) $(OBJ1) $(TARGET2) $(OBJ2)
