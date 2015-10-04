/*
Author: Most of the code was taken from "TCP/IP Sockets in C Practical Guide for Programmers, Second Edition", Donahoo and Calvert.

*/


#include <iostream>
#include <string>
#include <cstddef>
#include <sstream>
#include <netdb.h>                                                                                                                                                                                                                                              //For hostent
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>                                                                                                                                                                                                                                  //provides sockaddr_in and inet_addr()
#include <fstream>
#include <sys/time.h>
#include <netinet/tcp.h>
#include <sstream>
#include <istream>

using namespace std;

#include <stdio.h> /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h> /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h> /* for atoi() and exit() */
#include <string.h> /* for memset() */
#include <unistd.h> /* for close() */


#define MAXPENDING 5 /* Maximum outstanding connection requests */
#define BUFFSIZE 150   /* Size of receive buffer */
#define SERVPORT 1199	//Maximun ammount of entries on the list
#define FRAME_TYPE_SIZE 1
#define SEQ_NUM_SIZE 2
#define EOP_SIZE 1
#define PAYLOAD_SIZE 130
#define ERRD_SIZE 1

#define MAX_FRAME_PAYLOAD 130

class packet
{
	public:
		char payload[256]; 																											//256 bytes long
		char endPhoto;																												//1 byte long end of photo indicator
};

class frame
{
	public:
		short int seqNumber; 																										//2 bytes long
		char frameType; 																											//1 byte long
		char EOP;																													//1 byte long 
		char payload[130];																											//130 bytes long
		short int ED;																												//2 bytes long
};

void DieWithError(char *errorMessage); /* Error handling function */
frame* read_frame(char* buffer);
void printFrame (frame fr);

int main(int argc, char *argv[]){ 
 int bytes_r;
 int pid;
 char buffer[BUFFSIZE];
 int servSock; 					/*Socket descriptor for server */
 int clntSock; 					/* Socket descriptor for client */
 struct sockaddr_in ServAddr;		/* Local address */
 struct sockaddr_in ClntAddr; 		/* Client address */
 unsigned short ServPort; 			/* Server port */
 unsigned int clntLen;
 
 frame* inc_frame = new frame();
	ServPort = SERVPORT;			

 /* Create socket for incoming connections */
 if ((servSock = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
	 DieWithError("socket() failed");

/* Construct local address structure */
 memset(&ServAddr, 0, sizeof(ServAddr)); /* Zero out structure */
 ServAddr.sin_family = AF_INET; /* Internet address family */
 ServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
 ServAddr.sin_port = htons(ServPort); /* Local port */


 /* Bind to the local address */
 if (bind (servSock, (struct sockaddr *) &ServAddr, sizeof(ServAddr)) < 0)
	DieWithError("bind() failed");

	printf("Bind complete\n");			//Was here for debugging purpouses


 if (listen (servSock, MAXPENDING) < 0)
	 DieWithError("listen() failed");
	
	printf("Listening\n"); 				//Was here for debugging purpouses
	
for (;;) /* Run forever */{
 /* Set the size of the in-out parameter */
 clntLen = sizeof(ClntAddr); /* Wait for a client to connect */

 if ((clntSock = accept (servSock, (struct sockaddr *) &ClntAddr, &clntLen)) < 0)
	DieWithError("accept() failed");
	// printf("Accept\n"); 

 	printf("Handling client %s\n", inet_ntoa(ClntAddr.sin_addr));

	if((pid = fork()) < 0){
                printf("Error in fork()");
        }

        if (pid == 0) { // this is the child process
                close(servSock); // child doesn't need the listener
                if((bytes_r = recv(clntSock, buffer, BUFFSIZE - 1, 0)) < 0)
                        cout<< "Error in recv()";
				
                buffer[bytes_r] = '\0';
				cout<<"Bytes recieved: "<<bytes_r<<std::endl;
                cout<<"Buffer:\n"<<buffer<<std::endl;
                //Send AK
				
                if (send(clntSock, "ACK", 5, 0) == -1)
                        perror("send");
				
				
				inc_frame = read_frame(buffer);
				
				cout<<"Frame:\n"<<inc_frame->payload<<std::endl;
				
				printFrame(*inc_frame);
				
                close(clntSock);
                exit(0);
        }

        else if (pid > 0){
        close(clntSock);  // parent doesn't need this
    }

	
 	//HandleTCPClient(clntSock, echoBuffer, command);
 }
 /* NOT REACHED */
 }

void DieWithError(char *errorMessage)
{
    perror(errorMessage);
    exit(1);
} 

frame* read_frame(char* buffer){
	frame* new_frame = new frame();
	
	int i = 0;
	int place = 0;
	int startPayload = FRAME_TYPE_SIZE+SEQ_NUM_SIZE+EOP_SIZE;
	char *seq_num;
	char *frameType;
	char* EOP;
	char *ED;
	char payload[130];
	
	cout<<"Parsing sequence number...\n";
	while (i < place+SEQ_NUM_SIZE){
		cout<<i<<std::endl;
		cout<<"Starting copy..."<<std::endl;
		strncat(seq_num, &buffer[i], 1);
		i++;	
	}
	cout<<"Parsed sequence number...\n";
	
	place+=i;
	
	cout<<"Parsing frame type...\n";
	while (i< place+FRAME_TYPE_SIZE){
		strncat(frameType, &buffer[i], 1);
		i++;
	}
	cout<<"Parsed frame type...\n";
	
	place+=i;
	
	cout<<"Parsing payload...\n";
	while (i < place+PAYLOAD_SIZE){
		strncpy(&payload[i-startPayload], &buffer[i], 1);
		i++;
	}
	cout<<"Parsed payload...\n";
	
	place+=i;
	
	cout<<"Parsing Error Detection...\n";
	while(i < place+ERRD_SIZE){
		strncat(ED, &buffer[i], 1);
		i++;
	}
	cout<<"Parsed Error Detection...\n";
	
	
	new_frame->seqNumber = atoi(seq_num);
	
	strncpy(&new_frame->frameType, frameType, FRAME_TYPE_SIZE);
	
	strncpy(&new_frame->EOP, EOP, EOP_SIZE);
	
	strncpy(new_frame->payload, payload, PAYLOAD_SIZE - 1);
	new_frame->payload[PAYLOAD_SIZE] = '\0';
	
	new_frame->ED = atoi(ED);

	return new_frame;	
}

void printFrame (frame fr)																											//Alexi Kessler
{
	cout<<"Sequence Number: "<<fr.seqNumber<<"\nFrame Type: "<<fr.frameType<<"\nEnd of Packet Indicator: "
		<<fr.EOP<<std::endl;
	cout<<"Payload:"<<std::endl;
	int i = 0;
	
	while (i < MAX_FRAME_PAYLOAD)
	{
		cout<<"Frame Payload["<<i<<"]: "<<fr.payload[i]<<" acii value:"<<(int)fr.payload[i]<<std::endl;
		i++;
	}
	cout<<"Error Detection: "<<fr.ED<<std::endl;
}