CC = gcc
CFLAGS = -Wall -g -mavx2
BUILD_DIR = build

OBJ1 = $(BUILD_DIR)/4_1.o 


TARGET = $(BUILD_DIR)/4_1

all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(OBJ1)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ1)
$(BUILD_DIR)/4_1.o: 4_1.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)