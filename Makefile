CC = g++
CFLAGS = -c

all: chat

chat: main.o
 $(CC) main.o -o chat

main2.o: main.cpp
 $(CC) $(CFLAGS) main.cpp

clean:
 rm -rf *.o chat
