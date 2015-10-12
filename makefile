CC=g++
all: Client Server 

Client: client.cpp DieWithError.cpp FORCE
	$(CC) -Wall -g client.cpp DieWithError.cpp -o Client

Server: Server.cpp DieWithError.cpp FORCE
	$(CC) -Wall Server.cpp -o Server

FORCE:

clean:
	rm Client Server