CC=g++
FLAGS=-std=c++11 -O2 -Wall
LDLIBS=-lzmqpp -lzmq

all: broker client worker

broker: broker.cc
	$(CC) $(FLAGS) $(LDLIBS) -o broker broker.cc

client: client.cc
	$(CC) $(FLAGS) $(LDLIBS) -o client client.cc

worker: worker.cc
	$(CC) $(FLAGS) $(LDLIBS) -o worker worker.cc

clean:
	rm -f client broker worker
