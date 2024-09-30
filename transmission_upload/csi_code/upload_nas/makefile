LICHEE_DIR      := /data/lichee
SDK_LIB         :=
INCLUDE         +=
CFLAGS          += -g -Wall -O3 -fPIC  $(DEFINES) $(INCLUDE)
LIBS            += -lm
LDFLAGS         := -fPIC
CXXFLAGS        := $(CFLAGS)
SOURCE          := $(wildcard *.c) $(wildcard *.cpp)
OBJS            := $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SOURCE)))
TARGET          := upload_nas

CC              := gcc

.PHONY : everything objs clean distclean rebuild

all : $(TARGET)

objs : $(OBJS)

rebuild: distclean all

clean :
	rm -rf *~
	rm -rf *.o

distclean : clean
	rm -rf $(TARGET)

$(TARGET) : $(OBJS)
	$(CC) $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS) $(LIBS)
