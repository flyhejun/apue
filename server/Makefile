CC := gcc
CFLAGS := -Wall -g -I../common
LIBS := -lsqlite3 -lm

#静态库
LIB := ../lib/libmylib.a

TARGET := serv_prog
SOURCES := socket_server.c socket.c

all: $(LIB) $(TARGET)

$(TARGET): $(SOURCES) $(LIB)
	$(CC) $(CFLAGS) $(SOURCES) $(LIB) -o $(TARGET) $(LIBS)
	@echo "编译成功: $(TARGET)"

$(LIB):
	$(MAKE) -C ..

clean:
	rm -f $(TARGET)

.PHONY: all clean
