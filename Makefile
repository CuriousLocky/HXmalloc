CXX = gcc
ROOT_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
CFLAGS = -fPIC -fvisibility=hidden -std=gnu17 -mcx16 -ftls-model=initial-exec -march=native -nostdlib -Wall -flto -O3
CFLAGS += -I $(ROOT_DIR)/include -I $(ROOT_DIR)/generated/include

OBJ_DIR = obj
SRC_DIR = src
GEN_OBJ_DIR = obj/generated
GEN_SRC_DIR = generated/src

SRCS = $(wildcard src/*.c)
GEN_SRCS = $(wildcard generated/src/*.c)
DEPS = $(wildcard include/*.c) $(wildcard generated/include/*.c)
OBJ = $(SRCS:src/%.c=obj/%.o)
GEN_OBJ = $(GEN_SRCS:generated/src/%.c=obj/generated/%.o)

# all: $(OBJ_DIR) $(GEN_OBJ_DIR) HXmalloc.so
all: $(OBJ_DIR) HXmalloc.so

$(OBJ_DIR):
	@mkdir -p $@
$(GEN_OBJ_DIR):
	@mkdir -p $@

# HXmalloc.so: $(OBJ) $(GEN_OBJ)
HXmalloc.so: $(OBJ)
	@$(CXX) $(CFLAGS) -DRUNTIME -shared $(OBJ) $(GEN_OBJ) -o $@ -lpthread -ldl

HXmalloc.a: $(OBJ)
	ar rcs $@ $(OBJ)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@$(CXX) $(CFLAGS) -c $< -o $@

# $(GEN_OBJ_DIR)/%.o: $(GEN_SRC_DIR)/%.c
# 	@$(CXX) $(CFLAGS) -c $< -o $@

clean:
	@rm -rf obj HXmalloc.so generated