

CC = gcc
ECHO = echo

SUB_DIR = src/ sample/
ROOT_DIR = $(shell pwd)
OBJS_DIR = $(ROOT_DIR)/objs
BIN_DIR = $(ROOT_DIR)/bin

BIN = mm_server mm_client
FLAG = -lpthread -O3 -I $(ROOT_DIR)/src
EFLAG = -e ""

CUR_SOURCE = ${wildcard *.c}
CUR_OBJS = ${patsubst %.c, %.o, %(CUR_SOURCE)}

export CC BIN_DIR OBJS_DIR ROOT_IDR FLAG BIN ECHO EFLAG

all : $(SUB_DIR) $(BIN)
.PHONY : all


$(SUB_DIR) : ECHO
    make -C $@

#DEBUG : ECHO
#   make -C bin

ECHO :
    @echo $(SUB_DIR)


mm_server : $(OBJS_DIR)/mm_socket.o $(OBJS_DIR)/mm_coroutine.o $(OBJS_DIR)/mm_epoll.o $(OBJS_DIR)/mm_schedule.o $(OBJS_DIR)/mm_server.o
    $(CC) -o $(BIN_DIR)/$@ $^ $(FLAG) && $(ECHO) $(EFLAG)

mm_client : $(OBJS_DIR)/mm_socket.o $(OBJS_DIR)/mm_coroutine.o $(OBJS_DIR)/mm_epoll.o $(OBJS_DIR)/mm_schedule.o $(OBJS_DIR)/mm_client.o
    $(CC) -o $(BIN_DIR)/$@ $^ $(FLAG) && $(ECHO) $(EFLAG)

clean :
    rm -rf $(BIN_DIR)/* $(OBJS_DIR)/*

