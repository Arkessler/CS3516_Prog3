CC=g++
all: Client Server 

Client: client.cpp DieWithError.cpp
	$(CC) -Wall client.cpp DieWithError.cpp -o client

Server: Server.cpp DieWithError.cpp
	$(CC) -Wall Server.cpp -o Server

clean:
	rm -f client server