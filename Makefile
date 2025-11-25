CC = gcc
CFLAGS = -Wall -g -pthread

BUILD_DIR = build

OBJ1 = $(BUILD_DIR)/1_1.o 
OBJ2 = $(BUILD_DIR)/1_2.o
OBJ3 = $(BUILD_DIR)/1_3.o
OBJ4 = $(BUILD_DIR)/1_4.o
OBJ5 = $(BUILD_DIR)/1_5.o
OBJ5_2 = $(BUILD_DIR)/1_5_mutex_condvar.o
OBJ5_3 = $(BUILD_DIR)/1_5_busy_waiting.o

TARGET1 = $(BUILD_DIR)/1_1
TARGET2 = $(BUILD_DIR)/1_2
TARGET3 = $(BUILD_DIR)/1_3
TARGET4 = $(BUILD_DIR)/1_4
TARGET5 = $(BUILD_DIR)/1_5
TARGET5_2 = $(BUILD_DIR)/1_5_mutex_condvar
TARGET5_3 = $(BUILD_DIR)/1_5_busy_waiting

all: $(BUILD_DIR) $(TARGET1) $(TARGET2) $(TARGET3) $(TARGET5) $(TARGET4) $(TARGET5_2) $(TARGET5_3)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET1): $(OBJ1)
	$(CC) $(CFLAGS) -o $(TARGET1) $(OBJ1)

$(TARGET2): $(OBJ2)
	$(CC) $(CFLAGS) -o $(TARGET2) $(OBJ2)

$(TARGET3): $(OBJ3)
	$(CC) $(CFLAGS) -o $(TARGET3) $(OBJ3)

$(TARGET4): $(OBJ4)
	$(CC) $(CFLAGS) -o $(TARGET4) $(OBJ4)

$(TARGET5): $(OBJ5)
	$(CC) $(CFLAGS) -o $(TARGET5) $(OBJ5)

$(TARGET5_2): $(OBJ5_2)
	$(CC) $(CFLAGS) -o $(TARGET5_2) $(OBJ5_2)

$(TARGET5_3): $(OBJ5_3)
	$(CC) $(CFLAGS) -o $(TARGET5_3) $(OBJ5_3)

$(BUILD_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Run targets with example arguments

#2 threads, 20 iterations, mutex/rwlock/atomic
run2:
	$(TARGET2) 4 10000 1   
	$(TARGET2) 4 10000 2
	$(TARGET2) 4 10000 3

# size of array
run3:
	$(TARGET3) 1000000

# number of threads, number of iterations
run5:
	$(TARGET5) 4 1000000
	$(TARGET5_2) 4 1000000
	$(TARGET5_3) 4 1000000

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean run2 run3 run5