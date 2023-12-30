CC = gcc
CFLAGS = -Wall

SRC_DIR = src
BUILD_DIR = build

TARGET = $(BUILD_DIR)/tarsau

.PHONY: all clean

all: $(BUILD_DIR) $(TARGET)

$(TARGET): $(SRC_DIR)/tarsau.c
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)
	
