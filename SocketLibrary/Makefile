
LIB_NAME = SocketsModelica

# directories
BASE_DIR = .

SRC_DIR = $(BASE_DIR)/src
OBJ_DIR = $(BASE_DIR)/obj
BIN_DIR = $(BASE_DIR)/bin
HDR_DIR = $(BASE_DIR)/include

HEADERS = $(HDR_DIR)/*.h
INCLUDE = -I. -I$(HDR_DIR)

# objects
OBJ_LIBS = $(OBJ_DIR)/libSocketsModelica.o \
			$(OBJ_DIR)/Sockets.o \
			$(OBJ_DIR)/House.o \
			$(OBJ_DIR)/Timer.o

# library
LIB_DIR = $(BASE_DIR)/lib
LIB = $(LIB_DIR)/lib$(LIB_NAME).a

# tests

TEST_DIR_SRC = $(SRC_DIR)/test
TEST_DIR_OBJ = $(OBJ_DIR)/test
TEST_DIR_BIN = $(BIN_DIR)/test

TEST_OBJS = $(TEST_DIR_OBJ)/test_libSocketsModelica.o
TEST_BINS = $(TEST_DIR_BIN)/test_libSocketsModelica

# compiler and flags
STD = --std=c99
UNAME = $(shell uname)
ifeq ($(UNAME), Linux)
STD = --std=gnu99
endif

CC = gcc
CFLAGS_PROD = -DNDEBUG
CFLAGS = $(STD) --pedantic --pedantic-errors -Wall -Wno-unused $(INCLUDE)

# # # # # # # # # #
# main directives #
# # # # # # # # # #

.PHONY: all clean

# object files
$(OBJ_LIBS): $(HEADERS)
	mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) $(SRC_DIR)/$(@F:.o=.c) -c -o $@ 

$(TEST_OBJS): $(HEADERS)
	mkdir -p $(TEST_DIR_OBJ)
	$(CC) $(CFLAGS) $(TEST_DIR_SRC)/$(@F:.o=.c) -c -o $@

# library
$(LIB): $(OBJ_LIBS)
	mkdir -p $(LIB_DIR)
	ar r $@ $(OBJ_LIBS)
	ranlib $@

# binaries
$(TEST_BINS): $(LIB) $(TEST_OBJS)
	mkdir -p $(TEST_DIR_BIN)
	$(CC) $(CFLAGS) $(TEST_DIR_OBJ)/$(@F).o -L$(LIB_DIR) -l$(LIB_NAME) -o $@


# # # # # # # # # # #
# other  directives #
# # # # # # # # # # #

prod: CFLAGS += $(CFLAGS_PROD)
prod: all

all: $(LIB)

tests: $(TEST_BINS)
tests: all

clean: 
	rm -rf $(LIB_DIR)
	rm -rf $(BIN_DIR)
	rm -rf $(OBJ_DIR)

