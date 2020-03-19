# 1) Select cross toolchains
#CC=gcc
CROSS = /opt/gcc-4.4.4-glibc-2.11.1-multilib-1.0/arm-fsl-linux-gnueabi/bin/arm-fsl-linux-gnueabi-
#CROSS = /opt/myd_toolchains/gcc-linaro-4.9-2014.11-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-

CC = $(CROSS)gcc
STRIP = $(CROSS)strip
CFLAGS = -Wall -g -O2

# 2) What api ver you use now, just update don't move these files
TM_INC_DIR := /include/mercuryapi_include
INC_DIR := -I./include -I.$(TM_INC_DIR)

# 3) Select the mercuryapi dir, must reference to 1)
TM_LIB_PATH := ./lib/m28x_mercuryapi_1.31.3.36
#TM_LIB_PATH := ./lib/myd_mercuryapi_1.31.3.36

LIB_PATH := ./lib
LINK_LIB = -L$(LIB_PATH) -ldhcpcd -L$(TM_LIB_PATH) -lmercuryapi -L$(TM_LIB_PATH) -lltkc -L$(TM_LIB_PATH) -lltkctm
OBJ_DIR := ./obj
BIN_DIR := ./bin

EXEC = $(BIN_DIR)/reader
OBJS := $(OBJ_DIR)/reader.o \
		$(OBJ_DIR)/app_sys_setting.o \
		$(OBJ_DIR)/ind_cfg.o \
		$(OBJ_DIR)/ind_string.o \
		$(OBJ_DIR)/mid_msgq.o \
		$(OBJ_DIR)/mid_mutex.o \
		$(OBJ_DIR)/mid_net.o \
		$(OBJ_DIR)/mid_select.o \
		$(OBJ_DIR)/mid_task.o \
		$(OBJ_DIR)/mid_telnet.o \
		$(OBJ_DIR)/mid_timer.o \
		$(OBJ_DIR)/ringbuf.o \
		$(OBJ_DIR)/serialM6E.o \
		$(OBJ_DIR)/server_m6e.o \
		$(OBJ_DIR)/shell_play.o  \
		$(OBJ_DIR)/telnetd_port.o \
		$(OBJ_DIR)/osex_net.o \
		$(OBJ_DIR)/m6e_upgrade.o \
		$(OBJ_DIR)/gpio_init.o \
		$(OBJ_DIR)/m6e_init.o \

all: clean $(EXEC)

$(OBJ_DIR)/%.o: %.c $^
	$(CC) -c -o $@ $<

$(EXEC):$(OBJS)
	$(CC) $(INC_DIR) -o $@ $(OBJS) $(LINK_LIB) -lrt -lpthread 

clean:
	rm -f $(EXEC) $(OBJ_DIR)/*.o
