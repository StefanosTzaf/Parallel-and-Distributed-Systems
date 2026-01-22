CC = gcc
CFLAGS = -Wall -g -O2 -mavx2
BUILD_DIR = build

OBJ1 = $(BUILD_DIR)/4_1.o 


TARGET1 = $(BUILD_DIR)/4_1


all: $(BUILD_DIR) $(TARGET1)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET1): $(OBJ1)
	$(CC) $(CFLAGS) -o $(TARGET1) $(OBJ1)

$(BUILD_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)