SOURCE  := $(wildcard *.c)
OBJS    := $(patsubst %.c,%.o,$(SOURCE))

CC=echo $(notdir $<); gcc
CFLAG=-O -g -Wall -c
TARGET=Gesture
INCLUDE=-I./
LIB=-lpthread -lm


$(TARGET) : $(OBJS)
	$(CC) -g -o $@ $(OBJS) $(LIB) $(INCLUDE)
#$(OBJS):%.o:%.c
#	$(CC) $(CFLAG) $@ $< $(LIB) $(INCLUDE)
#$(TARGET):
#	$(CC) -o $(TARGET) mtGesture.o checkGesture.o touch.o queue.o log.o $(LIB) $(INCLUDE)
checkGesture.o:
	$(CC) $(CFLAG) checkGesture.c $(INCLUDE)
mtGesture.o:
	$(CC) $(CFLAG) mtGesture.c $(INCLUDE)	
touch.o:
	$(CC) $(CFLAG) touch.c $(INCLUDE)
queue.o:
	$(CC) $(CFLAG) queue.c $(INCLUDE)
log.o:
	$(CC) $(CFLAG) log.c $(INCLUDE)
#$(TARGET):
#	$(CC) -o $(TARGET) mtGesture.o checkGesture.o touch.o queue.o log.o $(LIB) $(INCLUDE)

clean:
	rm -rf *.o
	rm  -rf $(TARGET)
