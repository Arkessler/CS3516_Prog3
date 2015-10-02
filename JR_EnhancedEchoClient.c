/*
Author: Juan Rodriguez
Date: 09/01/2015

This program is a exact copy of the EchoClient.c program submitted for the basic version, modified to work according to the requirements of the assigment.
It is influenced, specially on the usage of the function getaddrinfo(), by code found at: http://man7.org/linux/man-pages/man3/getaddrinfo.3.html.

This is a client that connects to a server, sends some strings, recieves them back and prints them.
*/ 


 #include <stdio.h> 			/* for printf() and fprintf() */
 #include <sys/socket.h> 		/* for socket(), connect(), send(), and recv() */
 #include <arpa/inet.h> 		/* for sockaddr_in and inet_addr() */
 #include <stdlib.h> 			/* for atoi() and exit() */
 #include <string.h> 			/* for memset() */
 #include <unistd.h> 			/* for close() */
 #include <netdb.h>

#define RCVBUFSIZE 32 			/* Size of receive buffer */
 

void DieWithError(char *errorMessage); /* Error handling function */ 

int main(int argc, char *argv[])
 {
 struct addrinfo hints;
 struct addrinfo *result, *Pos_addr;
 int ErrName; 				//Variable to check if name wasnt found
 int sock; 				/* Socket descriptor */
 int StrCnt;				//Number of strings sent
 struct sockaddr_in echoServAddr; 	/* Echo server address */
 char *echoServPort; 			/* Echo server port */
 char *servName;			/* Server IP address (dotted quad) */

 char echoBuffer[RCVBUFSIZE]; 		/* Buffer for echo string */
 unsigned int echoStringLen; 		/* Length of string to echo */
 int bytesRcvd, totalBytesRcvd; 	/* Bytes read in single recv() and total bytes read */
 
  if ((argc < 3) || (argc > 8)){ 		/* Test for correct number of arguments */ 
 	fprintf(stderr, "Usage: %s <Server IP> <Echo Word> [<Echo Port>]\n",
 	argv[0]);
 	exit(1);
   	}

 servName = argv[1]; 			/* First arg: server IP address (dotted quad) */
 StrCnt = argc - 2;			//If the connection port is not specified, this will be the correct ammount of strings sent 

  if (argc == 8){
 	echoServPort = argv[7]; 	/* Use given port, if any */
	StrCnt--;			//If the port is specified, this is the correct amount of strings sent.
  }

  else
 	echoServPort = "1199"; 		/* 7 is the well-known port for the echo service */

 char *echoString[StrCnt + 1]; 		/* Array of strings to send to echo server with room for x strings + final two bytes*/

 int i;					//Loop counter

 for(i = 0; i < StrCnt; i++){
	echoString[i] = argv[i + 2];
}
	echoString[i] = "0x10 0x03";
/*Construct the hints struct for the lookup of the IP address*/ 
memset(&hints, 0, sizeof(struct addrinfo));
 hints.ai_family = AF_INET;		//Family    		
 hints.ai_socktype = SOCK_STREAM;  	//Socket type
 hints.ai_flags = 0;    		
 hints.ai_protocol = IPPROTO_TCP;       //Protocol     
 hints.ai_canonname = NULL;
 hints.ai_addr = NULL;
 hints.ai_next = NULL;


if((ErrName = getaddrinfo(servName, echoServPort, &hints, &result)) != 0){
	fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ErrName));
      	exit(1);
       	}

for(Pos_addr = result; Pos_addr != NULL; Pos_addr = Pos_addr->ai_next){
	
 	if ((sock = socket (Pos_addr->ai_family,Pos_addr->ai_socktype, Pos_addr->ai_protocol)) < 0){
	continue;					//If the socket couldnt be created correctly, continue to try with the next IP addresses.
	}
	
 	if (connect (sock, Pos_addr->ai_addr,Pos_addr->ai_addrlen) == 0)
		break;					//If connection was succesful, exit the loop and continue to send strings

	close(sock);					//If connection failed, close socket and try next IP address
 	}	

if(Pos_addr == NULL){					//If no address was found, exit the program
	DieWithError("Connection failed\n");
	}			

i = 0;  						//Reset loop counter

while(i <= StrCnt){
 echoStringLen = strlen(echoString[i]); 			/* Determine input length */
 
/* Send the string to the server */
 if (send (sock, echoString[i], echoStringLen, 0) != echoStringLen)
	DieWithError("send() sent a different number of bytes than expected");
	//printf("Sent:%s\n", echoString[i]); 			//Was here for debugging purpouses
 
/* Receive the same string back from the server */
 totalBytesRcvd = 0;


while (totalBytesRcvd < echoStringLen){

 /* Receive up to the buffer size (minus 1 to leave space for a null terminator) bytes from the sender */
 if ((bytesRcvd = recv(sock, echoBuffer, RCVBUFSIZE - 1, 0)) <= 0)
	DieWithError("recv() failed or connection closed prematurely");
 
 totalBytesRcvd += bytesRcvd; 				/* Keep tally of total bytes */
 echoBuffer[bytesRcvd] = '\0'; 				/* Terminate the string! */
 
 if( i < StrCnt){
 	printf("Received: "); 					/* Setup to print the echoed string */ 
 	printf("%s\n", echoBuffer); 				/* Print the echo buffer */
 }

 else{
 	printf("Done");}
}
 i++;							//Increase counter

}
 printf("\n"); 						/* Print a final linefeed */
 close (sock);
 exit(0);
}
 

void DieWithError(char *errorMessage)
{
    perror(errorMessage);
    exit(1);
}
