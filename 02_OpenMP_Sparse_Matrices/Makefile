CC = gcc
CFLAGS = -fopenmp -Wall -g

BUILD_DIR = build

OBJ1 = $(BUILD_DIR)/2_1.o
OBJ2 = $(BUILD_DIR)/2_2.o
OBJ3 = $(BUILD_DIR)/2_3.o

TARGET1 = $(BUILD_DIR)/2_1
TARGET2 = $(BUILD_DIR)/2_2
TARGET3 = $(BUILD_DIR)/2_3


all: $(BUILD_DIR) $(TARGET1) $(TARGET2) $(TARGET3) 

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET1): $(OBJ1)
	$(CC) $(CFLAGS) -o $(TARGET1) $(OBJ1)

$(TARGET2): $(OBJ2)
	$(CC) $(CFLAGS) -o $(TARGET2) $(OBJ2)

$(TARGET3): $(OBJ3)
	$(CC) $(CFLAGS) -o $(TARGET3) $(OBJ3)

$(BUILD_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) ./bench_results