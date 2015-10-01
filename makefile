CC=g++

client: client.cpp DieWithError.cpp
	$(CC) -Wall client.cpp DieWithError.cpp -o client