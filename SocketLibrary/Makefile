
LIB_NAME = SocketsModelica

# directories
BASE_DIR = .
MODELICA = ../Modelica

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
			$(OBJ_DIR)/Buffers.o \
			$(OBJ_DIR)/Timer.o

# library
LIB_DIR = $(BASE_DIR)/lib
LIB = $(LIB_DIR)/lib$(LIB_NAME).a
LIB_HDR = $(HDR_DIR)/lib$(LIB_NAME).h

# tests

TEST_DIR_SRC = $(SRC_DIR)/test
TEST_DIR_OBJ = $(OBJ_DIR)/test
TEST_DIR_BIN = $(BIN_DIR)/test

TEST_OBJS = $(TEST_DIR_OBJ)/test_libSocketsModelica.o \
			$(TEST_DIR_OBJ)/test_buffers.o
TEST_BINS = $(TEST_DIR_BIN)/test_libSocketsModelica \
			$(TEST_DIR_BIN)/test_buffers

# compiler and flags
STD = --std=c99
UNAME = $(shell uname)
ifeq ($(UNAME), Linux)
STD = --std=gnu99
endif

CC = gcc
CFLAGS_PROD = -DNDEBUG
CFLAGS = $(STD) --pedantic --pedantic-errors -Wall -Wno-unused -DNO_PHEV $(INCLUDE)

# # # # # # # # # #
# main directives #
# # # # # # # # # #

.PHONY: clean all

# object files
$(OBJ_LIBS): $(HEADERS)
	test -d $(OBJ_DIR) || mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) $(SRC_DIR)/$(@F:.o=.c) -c -o $@ -lm

$(TEST_OBJS): $(HEADERS)
	test -d $(TEST_DIR_OBJ) || mkdir -p $(TEST_DIR_OBJ)
	$(CC) $(CFLAGS) $(TEST_DIR_SRC)/$(@F:.o=.c) -c -o $@ -lm

# library
$(LIB): $(OBJ_LIBS)
	test -d $(LIB_DIR) || mkdir -p $(LIB_DIR)
	ar r $@ $(OBJ_LIBS)
	ranlib $@
	cp $(LIB) $(MODELICA)
	cp $(LIB_HDR) $(MODELICA)

# binaries
$(TEST_BINS): $(LIB) $(TEST_OBJS)
	test -d $(TEST_DIR_BIN) || mkdir -p $(TEST_DIR_BIN)
	$(CC) $(CFLAGS) $(TEST_DIR_OBJ)/$(@F).o -L$(LIB_DIR) -l$(LIB_NAME) -o $@ -lm


# # # # # # # # # # #
# other  directives #
# # # # # # # # # # #

prod: CFLAGS += $(CFLAGS_PROD)
prod: all

all: $(LIB)

tests: all
tests: $(TEST_BINS)

clean: 
	rm -rf $(MODELICA)/lib$(LIB_NAME).a $(MODELICA)/lib$(LIB_NAME).h
	rm -rf $(LIB_DIR)
	rm -rf $(BIN_DIR)
	rm -rf $(OBJ_DIR)

