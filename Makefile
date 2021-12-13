######################################
#
######################################
#source file
SOURCE  := $(wildcard src/*.c) $(wildcard src/*.cc)
OBJS    := $(patsubst %.c,%.o,$(patsubst %.cc,%.o,$(SOURCE)))

#target you can change test to what you want

TARGET  := test/test

#compile and lib parameter

CC      := g++
LIBS    :=
LDFLAGS := -lpmem -lpthread
DEFINES :=
INCLUDE := -I.
CFLAGS  := -std=c++11 -Wall -o3 $(DEFINES) $(INCLUDE)
CXXFLAGS:= $(CFLAGS) 

#i think you should do anything here

%.o:%.cc %.h
	$(CC) $(CFLAGS) -o $@ -c $< 

.PHONY : objs clean  rebuild

all : $(TARGET)

objs : $(OBJS)

rebuild: clean all

clean :
	rm -f *.so
	rm -f *.o
	rm -f $(TARGET)

$(TARGET) : $(OBJS)
	$(CC) $(CXXFLAGS) -o $@  test/test.cc $(OBJS) $(LDFLAGS) $(LIBS)