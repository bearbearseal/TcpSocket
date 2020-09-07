OBJS = main.o TcpSocket.o TcpListener.o
CC = g++ -std=c++17
CFLAGS = -Wall -c
LFLAGS = -Wall
LIBS = -lpthread
#INC = -I../Basic
binaries = theMain.out

main : $(OBJS)
	$(CC) $(LFLAGS) $(OBJS) $(LIBS) -o $(binaries)

main.o : main.cpp Test_Socket.hpp
	$(CC) $(CFLAGS) main.cpp

TcpSocket.o : TcpSocket.h TcpSocket.cpp
	$(CC) $(CFLAGS) TcpSocket.cpp

TcpListener.o : TcpListener.h TcpListener.cpp TcpSocket.h
	$(CC) $(CFLAGS) TcpListener.cpp

clean:
	rm -f $(binaries) *.o
