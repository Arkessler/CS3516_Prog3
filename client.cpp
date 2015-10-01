#include <iostream>
#include <string>
#include <cstddef>
#include <sstream>
#include <netdb.h> 																														//For hostent
//#hi
#include <unistd.h> 
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>  																												//provides sockaddr_in and inet_addr()
#include <fstream>
#include <sys/time.h>
#include <netinet/tcp.h> 
#include <sstream>
#include <istream>

#define CHUNK_SIZE 256
#define PACKET_SZIE CHUNK_SIZE+1
#define MAX_FRAME_PAYLOAD 130

using namespace std;

//typedef enum {frame_arrival} event_type

//Debug variable(s)
bool DEBUG = true;
//Misc global var(s)
std::ofstream fileStream;

//Network layer functions
void nwl_read(std::string fileName);
void nwl_recv();
//Data Link layer functions
void dll_send();
void dll_recv();
//Physical layer functions
int phl_connect(int *sock, struct sockaddr_in serverAddress, unsigned short serverPort, char *serverName);
void phl_send();
void phl_recv();
void phl_close(int sock);
//Misc functions
void DieWithError(char *errorMessage);

int main(int argc, char* argv[])
{
	//Command line variables
	char* serverName;
	std::string portString = "";
	int cId = 0;
	int numPhoto = 0;
	
	//Connection variables
	int sockfd;
	struct sockaddr_in serverAddress;
	unsigned short portNumber = 1099; 																					//Assuming use of "well-known" port
	
	//Misc variables
	std::stringstream manipStream;
	
	//------------------------------------------------------------------------INIT VARIABLES---------------------------------------------------------------------------
	serverName = argv[1];
	cId = atoi(argv[2]);
	numPhoto = atoi(argv[3]);
	
	//------------------------------------------------------------------------BEGIN PHOTO READ----------------------------------------------------------------------------
	int count = 0;
	
	std::string readLoc; 
	
	int connectRes = phl_connect(&sockfd, serverAddress, portNumber, serverName);
	if (DEBUG)
		cout<<"Phl_connect returned: "<<connectRes<<std::endl;
	
	for (count = 0; count<numPhoto; count++)
	{
		//Set new readLoc
		readLoc = "photo";
		manipStream<<cId;																			//Convert cId to string
		readLoc.append(manipStream.str());
		manipStream.str(std::string());																//Reset manipStream
		manipStream<<count;																			//Convert count to string
		readLoc.append(manipStream.str());   														//THIS MAY NEED TO BE CHANGED TO 1 INSTEAD OF COUNT
		manipStream.str(std::string());																//Reset manipStream
		readLoc.append(".jpg");
		if (DEBUG)
			cout<< "New read location: "<<readLoc<<std::endl;
		//Have app layer read file
		nwl_read(readLoc);
	}
	
	phl_close(sockfd);
}

class packet
{
	char payload[256]; 																//256 bytes long
	char endPhoto		;															//1 byte long end of photo indicator
} packet;

class frame
{
	short int seqNumber; 															//2 bytes long
	char frameType; 																//1 byte long
	char EOP;																		//1 byte long 
	char payload[130];																//130 bytes long
	short int XOR;																	//2 bytes long
} frame;

void nwl_read(std::string fileName)
{
	unsigned char buf[300];
	char currChar;
	//iStream.open(&fileName, std::ifstream::in);
	while (iStream.read(buf, 256))
	{
		packet newPacket = new packet();
	}
		//Construct packet
		//dll_send(packet)
		//Wait on nwl_ACK
	//If buf not empty
		//Construct packet
		//dll_send(packet)
		//Wait on nwl_ACK;
}

void nwl_recv()
{
	
}

void dll_send()
{
	//Read packet
	//Separate into payloads
	//For each payload
		//Create frame
		//Start_timer
		//Phl_send(frm)
		//Phl_recv(ack/frm)
		//If ack
			//If ok 
				//increment
			//Else
				//Retransmit
		//If frm
			//return nwl_ack
	//Wait on nwl_ack
}

void dll_recv()              // ----------------------WHEN WOULD WE USE THIS?
{
	
}

int phl_connect(int *sock, struct sockaddr_in serverAddress, unsigned short serverPort, char *serverName)
{
	struct addrinfo hints, *servinfo, *p;
	int rv;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; 																										// use AF_INET6 if desire is to force IPv6
	hints.ai_socktype = SOCK_STREAM;
	
	if ((rv = getaddrinfo(serverName, "http", &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		exit(1);
	}
																							
	for(p = servinfo; p != NULL; p = p->ai_next)
	{																						//loop through all the results and connect to the first we can		
		if ((*sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) 
		{
			perror("socket");
			continue;
		}
		memset(&serverAddress, 0, sizeof(serverAddress));  																		//Fill struct with zeroes
		serverAddress.sin_family  = AF_INET;    																					//Set Internet address family
		serverAddress.sin_addr.s_addr =inet_addr(inet_ntoa((struct in_addr) ((struct sockaddr_in *)(p->ai_addr))->sin_addr));  		//Set server IP address using result from getaddrinfo()
		serverAddress.sin_port = htons(serverPort);																				//Set server port. Use well-known port due to getaddrinfo() returning port 80																															//Connect to echo server
		if (connect(*sock, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0) 										//Test that connection is successful
			return -1;
		else
			return 0;
	}
	return -2; //Should not reach this point
}

void phl_send()
{
	//Send over TCP connection
	//phl_recv()
}

void phl_recv()
{
	//recv() wait
}

void phl_close(int sock)
{
	close(sock);
}

void waitEvent()
{
	
}

