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
#define BUFFSIZE 32   /* Size of receive buffer */
#define SERVPORT 1099	//Maximun ammount of entries on the list


void DieWithError(char *errorMessage); /* Error handling function */


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

	ServPort = SERVPORT;			//If no port is specified, use this one that is generally available and matches with the one of the client program

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

                //Send AK

                if (send(clntSock, "ACK", 5, 0) == -1)
                        perror("send");

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

/*
void getCommand(char *buffer, char *storeBuf){
        int wordLen = 0;                                                        //Loop counter
        while(buffer[wordLen] != 32){
                wordLen++;}
        strncpy(storeBuf, buffer, wordLen);
        storeBuf[wordLen] = '\0';
}

void add(char* id, char* fname, char* lname, char* gender, char* location){	
	int i;
	int j;
	entry temp;
	for(i = 0; i < LSIZE; i++){
		temp = entry[i];
		if(strcmp(temp->id, id) == 0){
		//Person already in list
		return;
		}
	}
		//Add
		&temp->id = id;
		&temp->fname = fname;
		&temp->lname = lname;
		&temp->gender = gender;
		&temp->location = location;	
		
	
		//Sort
		for (i = 0; i < LSIZE; i++) {
      			for (j = 0; j < LSIZE - 1; j++) {
         			if (strcasecmp(entry[j].lname, entry[j + 1].lname) > 0) {
            				memcpy(temp, entry[j], sizeof(entry));
					memcpy(entry[j], entry[j +1], sizeof(entry));
					memcpy(entry[j+1], temp, sizeof(entry));
				}
			}
		}		
}


void update(char* id, char* fname, char* lname, char* gender, char* location){
        entry temp;
	temp = malloc(sizeof(entry));
	for(i = 0; i < LSIZE; i++){
                memcpy(temp,entry[i], sizeof(entry));
                if(strcmp(temp->id, id) == 0){
                //Person already in list
                &temp->id = id;
                &temp->fname = fname;
                &temp->lname = lname;
                &temp->gender = gender;
                &temp->location = location;
		 return;
                }
	
		else(){
		//entry not found
		}
	}
	free(temp);	
}

void remove(char* id){
	int i;
	entry temp;
	temp = malloc(sizeof(entry));
        for(i = 0; i < LSIZE; i++){
                temp = entry[i];
                if(strcmp(temp->id, id) == 0){
			memset(&temp, 0, sizeof(entry));
                	return;
		}
	}
free(temp);
}

char* find(char* fname, char* lname){
	int i;
	entry temp;
	temp = malloc(sizeof(entry));
	
	for(i = 0; i < LSIZE; i++){
	        temp = entry[i];
                
		if(strcmp(temp->lname, lname) == 0){
			if(strcmp(temp->fname, fname) == 0){
				char x = " ";
				int strlen = strlen(x) + 1;
				char *temp = strcat(fname, x, strlen);
				char *buffer = malloc(sizeof(char) * (strlen(fname) + strlen(lname) + 1));				
				return buffer = strncat(temp, lname, (strlen(lnane) + 1));  
                	}
		}		

	}
	free(temp);
}

void locate(char* location){
        int i;
        entry temp;
        temp = malloc(sizeof(entry));

        for(i = 0; i < LSIZE; i++){
                temp = entry[i];

                if(strcasecmp(temp->location, location) == 0){

}

void list(char* start, char* finish){

}

void sortFunc(struct entry){
	entry temp;
	temp = malloc(sizeof(entry));
	int i, j;
                for (i = 0; i < LSIZE; i++) {
                        for (j = 0; j < LSIZE - 1; j++) {
                                if (strcasecmp(entry[j].lname, entry[j + 1].lname) > 0) {
                                        memcpy(temp, entry[j], sizeof(entry));
                                        memcpy(entry[j], entry[j +1], sizeof(entry));
                                        memcpy(entry[j+1], temp, sizeof(entry));
                                }
                        }
                }
	free(temp);
}
*/
