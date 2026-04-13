CC := gcc
CFLAGS := -Wall -g -Icommon
AR := ar
ARFLAGS := rcs

SRC_DIR = common
OBJ_DIR = obj
LIB_DIR = lib

LIB_NAME := libmylib.a
LIB_TARGET =$(LIB_DIR)/$(LIB_NAME)

SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SOURCES))

all: $(LIB_TARGET)

$(LIB_TARGET): $(OBJS) | $(LIB_DIR) 
	$(AR) $(ARFLAGS) $@ $^
	@echo "静态库创建成功: $@"
	@echo "包含的文件:"
	@ar t $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $@

$(LIB_DIR):
	mkdir -p $@

clean:
	@rm -rf $(OBJ_DIR) $(LIB_DIR)

.PHONY: all clean 
