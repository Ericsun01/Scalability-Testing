CPPFLAGS=-std=c++11 -Wall -pedantic -lpthread -pthread

all: server client

server: server.cpp
	g++ -o server $(CPPFLAGS) server.cpp
client: client.cpp
	g++ -o client $(CPPFLAGS) client.cpp

# clean:
#  	rm -rf server client *.o *~