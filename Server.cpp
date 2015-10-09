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
#define SEQ_NUM_SIZE 5
#define EOP_SIZE 1
#define PAYLOAD_SIZE 130
#define ERRD_SIZE 6
#define MAX_FRAME_PAYLOAD 130
#define USABLE_BYTES 5

#define DEBUG 0

class packet
{
	public:
		char payload[256]; 																											//256 bytes long
		char endPhoto;																												//1 byte long end of photo indicator
};

class frame
{
	public:
		unsigned short int seqNumber; 																										//2 bytes long
		char frameType; 																											//1 byte long
		char EOP;
		unsigned short int usable_bytes;																													//1 byte long 
		char payload[MAX_FRAME_PAYLOAD];																											//130 bytes long
		short int ED;																												//2 bytes long
};

void DieWithError(char *errorMessage); /* Error handling function */
frame* read_frame(char* buffer);
void printFrame (frame fr);

int main(int argc, char *argv[]){ 
 int bytes_r;
 int pid;
 int i;								//Loop counter
 char buffer[BUFFSIZE];
 int servSock; 					/*Socket descriptor for server */
 int clntSock; 					/* Socket descriptor for client */
 struct sockaddr_in ServAddr;		/* Local address */
 struct sockaddr_in ClntAddr; 		/* Client address */
 unsigned short ServPort; 			/* Server port */
 unsigned int clntLen;
 frame* tempPacket[10];
 char* test_string = "Hello Master";
 
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
        		i = 0;
                close(servSock); // child doesn't need the listener
                do{

                	memset(buffer, 0, sizeof(buffer));

                	if((bytes_r = recv(clntSock, buffer, BUFFSIZE - 1, 0)) < 0)
                	        cout<< "Error in recv()";
				
                	//buffer[bytes_r] = '\0';
					cout<<"Bytes recieved: "<<bytes_r<<std::endl;
                	
                	int x = 0;
                	while(x < BUFFSIZE){
                	cout<<"Buffer["<<x<<"]: "<<buffer[x]<<" ascii val: "<<(int) buffer[x]<<std::endl;
                	x++;
                	}
                	//Send AK
					inc_frame = read_frame(buffer);
					//if(inc_frame != NULL){
						cout<<"Test String: "<<(test_string + i)<<std::endl;

                		if (send(clntSock, test_string + i, strlen(test_string + i), 0) == -1)
                    	    	perror("send");
					//}

					tempPacket[i] = inc_frame;					//Store the frames to later build up 
					i++;

					cout<<"Frame:\n"<<inc_frame->payload<<std::endl;
					cout<<"i value: "<<i<<std::endl;

					//printFrame(*inc_frame);


        		} while(&inc_frame->EOP != "E");

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
	int startFrameType = SEQ_NUM_SIZE;
	int startEOP = startFrameType + FRAME_TYPE_SIZE;
	int startUsableBytes = startEOP + EOP_SIZE;
	int startPayload = startUsableBytes + USABLE_BYTES;
	int startERRD = startPayload + PAYLOAD_SIZE;
	char seq_num[SEQ_NUM_SIZE + 1];
	int ED_temp;
	char frameType;
	char usable_b[USABLE_BYTES + 1];
	char EOP;
	char ED[ERRD_SIZE + 1];
	char payload[130];
		
	memset(seq_num, 0, strlen(seq_num));
	
	cout<<"Parsing sequence number...\n";
	while (i < place+SEQ_NUM_SIZE){
		
		if(DEBUG){
		cout<<"Value of i: "<<i<<"\nValue of place: "<<place<<std::endl;
		cout<<"Starting copy..."<<std::endl;
		}
		strncat(&seq_num[i], &buffer[i], 1);
		
		if(DEBUG){
		cout<<"\nValue of SEQ_NUM: "<<seq_num<<std::endl;
		}
		
		i++;	
	}
	seq_num[SEQ_NUM_SIZE] = '\0';
	cout<<"Parsed sequence number: "<<seq_num<<std::endl;
	
	if(DEBUG){
	cout<<"Value i: "<<i<<std::endl;
	}
	
	place=i;
	
	cout<<"Parsing frame type...\n";
	while (i< place+FRAME_TYPE_SIZE){
		strncat(&new_frame->frameType, &buffer[i], 1);
		i++;
	}
	cout<<"Parsed frame type: "<<frameType<<std::endl;
	
	if(DEBUG){
	cout<<"Value i: "<<i<<"\nPlace: "<<place<<std::endl;
	}
	
	place=i;
	
	cout<<"Parsing EOP...\n";
	while (i < place+EOP_SIZE){
		
		if(DEBUG){
		cout<<"EOP while 'i' value: "<<i<<std::endl;
		}
		
		strncpy(&new_frame->EOP, &buffer[i], 1);
		i++;
	}
	cout<<"Parsed EOP...\n";
	 place =i;
	
	if(DEBUG){
	cout<<"Value i: "<<i<<"\nPlace: "<<place<<std::endl;
	}
	
	cout<<"Parsing usable bytes...\n";
	while (i < place+USABLE_BYTES){
		
		if(DEBUG){
		cout<<"EOP while 'i' value: "<<i<<std::endl;
		}
		
		strncpy(&usable_b[i-startUsableBytes], &buffer[i], 1);
		i++;
	}
	usable_b[USABLE_BYTES] = '\0';
	cout<<"Printing usable_b string: "<<usable_b<<std::endl;

	cout<<"Parsed usable bytes...\n";
	
	place=i;
	
	cout<<"Parsing payload...\n";

	int bytes_rcvd = atoi(usable_b);
	cout<<bytes_rcvd<<std::endl;

	while (i < place+PAYLOAD_SIZE){
		
		if(DEBUG){
		cout<<"Value i: "<<i<<std::endl;
		}
		
		strncpy(&new_frame->payload[i-startPayload], &buffer[i], 1);
		i++;
		
		if(DEBUG){
		cout<<"Value i: "<<i<<std::endl;
		cout<<"Payload["<<i-startPayload<<"] "<<&payload[i-startPayload]<<std::endl;
		}
	}
	//memset(&new_frame->payload[bytes_rcvd], 0, PAYLOAD_SIZE - bytes_rcvd);
	cout<<"Parsed payload...\n";
	
	place=i;
	
	if(DEBUG){
	cout<<"Value i: "<<i<<"\nPlace: "<<place<<std::endl;
	}
	
	cout<<"Parsing Error Detection...\n";
	while(i < place+ERRD_SIZE){
		strncpy(&ED[i-startERRD], &buffer[i], 1);
		i++;
	}
	cout<<"Error Detection string: "<<ED<<std::endl;
	ED[ERRD_SIZE] = '\0';

	int parse = 0;
	int ignore = 0;
	while(parse < ERRD_SIZE){ 
		if(ED[parse] == '0'){
			ignore++;
		}

		else if(ED[parse] == '-'){
			break;
		}
		
		parse++;
	}

	cout<<"Ignore: "<<ignore<<std::endl;
	
	if(DEBUG){
	cout<<"Parsed Error Detection: "<<ED<<std::endl;
	}
	
	new_frame->seqNumber = atoi(seq_num);
	/***********************************NEEDS TO BE CHECKED****************************/
	//new_frame->payload[PAYLOAD_SIZE] = '\0';
	
	new_frame->usable_bytes = bytes_rcvd;
	new_frame->ED = atoi(&ED[ignore]);
	
	printFrame(*new_frame);
	return new_frame;	
		
}

void printFrame (frame fr)																											//Alexi Kessler
{
	cout<<"Sequence Number: "<<fr.seqNumber<<"\nFrame Type: "<<fr.frameType<<"\nEnd of Packet Indicator: "
		<<fr.EOP<<std::endl;
	cout<<"Usable bytes: "<<fr.usable_bytes<<std::endl;
	/*
	cout<<"Payload:"<<std::endl; 

	int i = 0;
	
	while (i < MAX_FRAME_PAYLOAD) 
	{
		cout<<"Frame Payload["<<i<<"]: "<<fr.payload[i]<<" acii value:"<<(int)fr.payload[i]<<std::endl;
		i++;
	}
	*/
	cout<<"Error Detection: "<<fr.ED<<std::endl;
	
}