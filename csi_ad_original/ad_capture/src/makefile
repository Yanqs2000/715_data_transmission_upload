LICHEE_DIR     := /home/tl/workspace/relase_space/LinuxSDK
LICHEE_OUT_SYS  = $(LICHEE_DIR)/out/t3/tlt3f-evm/buildroot
HOST_SYSROOT    = $(LICHEE_OUT_SYS)/buildroot/host/arm-buildroot-linux-gnueabihf/sysroot
CC              = $(LICHEE_OUT_SYS)/buildroot/host/bin/arm-linux-gnueabihf-gcc --sysroot=$(HOST_SYSROOT)

SDK_LIB 	:=
INCLUDE 	+=
CFLAGS  	+= -g -Wall -O3 $(INCLUDE)
LIBS    	+=
LDFLAGS 	:= -lpthread
CXXFLAGS	:= $(CFLAGS)
SOURCE  	:= $(wildcard *.c) $(wildcard *.cpp)
OBJS    	:= $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SOURCE)))
TARGET  	:= ad_capture

.PHONY : everything objs clean distclean rebuild

all : $(TARGET)

objs : $(OBJS)

rebuild: distclean all

clean :
	rm -rf *~
	rm -rf *.o
	rm -rf $(TARGET)

$(TARGET) : $(OBJS)
	$(CC) $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS) $(LIBS)
