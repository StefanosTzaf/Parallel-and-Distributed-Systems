CC = gcc
CFLAGS = -Wall -g -mavx2
CFLAGS_OPT = -Wall -g -mavx2 -O2
BUILD_DIR = build

OBJ1 = $(BUILD_DIR)/4_1.o
OBJ1_OPT = $(BUILD_DIR)/4_1_O2.o

TARGET = $(BUILD_DIR)/4_1
TARGET_OPT = $(BUILD_DIR)/4_1_O2

all: $(BUILD_DIR) $(TARGET) $(TARGET_OPT)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(OBJ1)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ1)

$(TARGET_OPT): $(OBJ1_OPT)
	$(CC) $(CFLAGS_OPT) -o $(TARGET_OPT) $(OBJ1_OPT)

$(BUILD_DIR)/4_1.o: 4_1.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/4_1_O2.o: 4_1.c
	$(CC) $(CFLAGS_OPT) -c $< -o $@
	
clean:
	rm -rf $(BUILD_DIR)