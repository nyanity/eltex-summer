CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -Werror -std=gnu17 -I. -MMD -MP
LDFLAGS = 

BUILD_DIR = build
TARGET = $(BUILD_DIR)/taxi

C_SRCS = cli.c driver.c
C_OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SRCS))

DEPS = $(C_OBJS:.o=.d)

all: $(TARGET)

$(TARGET): $(C_OBJS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean

-include $(DEPS)