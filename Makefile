CC=g++
FLAGS=-std=c++11 -O2 -Wall
LDLIBS=-lzmqpp -lzmq -lpthread -lsfml-audio -luuid
PTH=./src

all: broker client server

broker: $(PTH)/broker.cc
	$(CC) $(FLAGS) $(LDLIBS) -o broker $(PTH)/broker.cc

client: $(PTH)/client.cc
	$(CC) $(FLAGS) $(LDLIBS) -o client $(PTH)/client.cc

server: $(PTH)/server.cc
	$(CC) $(FLAGS) $(LDLIBS) -o server $(PTH)/server.cc

clean:
	rm -f client broker server
