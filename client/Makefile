CC := gcc
CFLAGS := -Wall -g -I/home/iot26/hejunfei/common
LIBS := -lsqlite3 -lm

#静态库路径
LIB_DIR := /home/iot26/hejunfei/lib
LIB := $(LIB_DIR)/libmylib.a

# 源文件
CLIENT_SRCS := ds18b20.c socket_client.c
TARGET := cli_prog

all:
	$(CC) $(CFLAGS) $(CLIENT_SRCS) $(LIB) -o $(TARGET) $(LIBS)
	@echo "✓ 编译成功: $(TARGET)"

clean:
	rm -f $(TARGET)

.PHONY: all clean
