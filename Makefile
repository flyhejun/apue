CC := gcc
CFLAGS := -Wall -g -Icommon -Iclient
AR := ar
ARFLAGS := rcs

OBJ_DIR = obj
LIB_DIR = lib

LIB_NAME := libmylib.a
LIB_TARGET = $(LIB_DIR)/$(LIB_NAME)

OBJS = $(OBJ_DIR)/cJSON.o $(OBJ_DIR)/database.o $(OBJ_DIR)/log.o $(OBJ_DIR)/packet.o

all: $(LIB_TARGET)

$(LIB_TARGET): $(OBJS) | $(LIB_DIR)
	$(AR) $(ARFLAGS) $@ $^
	@echo "静态库创建成功: $@"
	@echo "包含的文件:"
	@ar t $@

$(OBJ_DIR)/cJSON.o: common/cJSON.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/database.o: common/database.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/log.o: common/log.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/packet.o: common/packet.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $@

$(LIB_DIR):
	mkdir -p $@

clean:
	@rm -rf $(OBJ_DIR) $(LIB_DIR)

.PHONY: all clean
