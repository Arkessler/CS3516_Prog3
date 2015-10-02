CC=g++

client: client.cpp DieWithError.cpp
	$(CC) -Wall client.cpp DieWithError.cpp -o client

server: server.cpp DieWithError.cpp
	$(CC) -Wall server.cpp DiewWithError.cpp -o server

