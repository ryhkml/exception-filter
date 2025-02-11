CFLAGS = -O2 -Wall -Wextra -Wformat -Wformat-security \
         -fstack-protector-strong -D_FORTIFY_SOURCE=2 \
         -pipe -std=c11 -MMD -MP -pthread

SRC_DIR = src
OUT_DIR = out
TARGET = exception-filter

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OUT_DIR)/%.o, $(SRCS))
DEPS = $(patsubst $(SRC_DIR)/%.c, $(OUT_DIR)/%.d, $(SRCS))

.PHONY: all clean

all: $(OUT_DIR)/$(TARGET)

$(OUT_DIR)/$(TARGET): $(OBJS) | $(OUT_DIR)
	$(CC) $(CFLAGS) -o $@ $^

$(OUT_DIR)/%.o: $(SRC_DIR)/%.c | $(OUT_DIR)
	$(CC) $(CFLAGS) -c -o $@ $< -MF $(OUT_DIR)/$*.d

clean:
	rm -rf $(OUT_DIR)/$(TARGET) $(OUT_DIR)/*.d $(OUT_DIR)/*.o

-include $(DEPS)
