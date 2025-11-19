OBJS = 1_1.o 
CC = gcc
CFLAGS = -Wall -g -pthread
TARGET = 1_1



$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)
clean:
	rm -f $(TARGET) $(OBJS)
