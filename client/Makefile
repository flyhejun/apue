CC := gcc
CFLAGS := -Wall -g -I../common
LIBS := -lsqlite3 -lm

#静态库路径
LIB_DIR := ../lib
LIB := $(LIB_DIR)/libmylib.a

# 源文件
CLIENT_SRCS := ds18b20.c socket_client.c socket.c
TARGET := cli_prog

all: $(LIB) $(TARGET)

$(TARGET): $(CLIENT_SRCS) $(LIB)
	$(CC) $(CFLAGS) $(CLIENT_SRCS) $(LIB) -o $(TARGET) $(LIBS)
	@echo "编译成功: $(TARGET)"

$(LIB):
	$(MAKE) -C ..

clean:
	rm -f $(TARGET)

.PHONY: all clean
