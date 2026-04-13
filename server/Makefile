C := gcc
CFLAGS := -Wall -g -I/home/iot26/hejunfei/common
LIBS := -lsqlite3 -lm

#静态库（关键！）
LIB := ../lib/libmylib.a

TARGET := serv_prog
SOURCES := socket_server.c

all:
	$(CC) $(CFLAGS) $(SOURCES) $(LIB) -o $(TARGET) $(LIBS)
	@echo "✓ 编译成功: $(TARGET)"

clean:
	rm -f $(TARGET)

.PHONY: all clean
