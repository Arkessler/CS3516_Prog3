#include <iostream>
#include <string>
#include <cstddef>
#include <sstream>
#include <netdb.h> 																													//For hostent
#include <unistd.h> 
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>  																											//provides sockaddr_in and inet_addr()
#include <fstream>																	
#include <netinet/tcp.h> 
#include <sstream>																													//Provides stringStream
#include <fstream>																													//Provides ofstream, read()
#include <errno.h>

#define DEFAULT_PORT 1199
#define MAX_SIZE_PACKET 257
#define MAX_SIZE_PACKET_PAYLOAD 256
#define PACKET_SIZE MAX_SIZE_PACKET_PAYLOAD+1
#define MAX_FRAME_PAYLOAD 130
#define MAX_FRAME_SIZE 200
#define DATA_FRAME 'D'
#define ACK_FRAME 'A'
#define END_PACKET 'E'
#define CONT_PACKET 'C'
#define MAX_WAIT_TIME 750

using namespace std;

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

//typedef enum {frame_arrival} event_type

//Debug variable(s)
bool DEBUG = true;
//Send global var(s)
int sockfd = 0;
short int sequenceNumber = 0;
//Misc global var(s)
std::ofstream fileStream;

//Network layer functions
void nwl_read(std::string fileName);
void nwl_recv();
//Data Link layer functions
void dll_send(packet pkt);
void dll_recv();
//Physical layer functions
int phl_connect(struct sockaddr_in serverAddress, unsigned short serverPort, char *serverName);
void phl_send(frame fr);
void phl_recv();
void phl_close();
//Misc functions
void DieWithError(char *errorMessage);
short int errorDetectCreate(short int seq, char frameType, char EOP, char payload[]);
void testSendMessage();
void testSendFrame();
void printFrame (frame fr);
int waitEvent();

int main(int argc, char* argv[])																									//Alexi Kessler
{
	//Command line variables
	char* serverName;
	std::string portString = "";
	int cId = 0;
	int numPhoto = 0;
	
	//Connection variables
	struct sockaddr_in serverAddress;
	unsigned short portNumber = DEFAULT_PORT; 																						//Assuming use of "well-known" port
	
	//Misc variables
	std::stringstream manipStream;
	
	//------------------------------------------------------------------------INIT VARIABLES---------------------------------------------------------------------------
	serverName = argv[1];
	cId = atoi(argv[2]);
	numPhoto = atoi(argv[3]);
	
	//------------------------------------------------------------------------BEGIN PHOTO READ---------------------------------------------------------------------------
	
	int count = 0;
	
	std::string readLoc; 

	int connectRes = phl_connect(serverAddress, portNumber, serverName);
	if (DEBUG)
		cout<<"Phl_connect returned: "<<connectRes<<std::endl;
	
	//testSendMessage();
	testSendFrame(); 
	
	for (count = 0; count<numPhoto; count++)
	{
																																	//Set new readLoc
		readLoc = "photo";
		manipStream<<cId;																											//Convert cId to string
		readLoc.append(manipStream.str());
		manipStream.str(std::string());																								//Reset manipStream
		manipStream<<count;																											//Convert count to string
		readLoc.append(manipStream.str());   																						//THIS MAY NEED TO BE CHANGED TO 1 INSTEAD OF COUNT
		manipStream.str(std::string());																								//Reset manipStream
		readLoc.append(".jpg");
		if (DEBUG)
			cout<< "New read location: "<<readLoc<<std::endl;
		nwl_read(readLoc);																											//Have network layer read file
	}
	
	phl_close();
}

void nwl_read(std::string fileName)																									//Alexi Kessler
{
	std::ifstream stream (fileName.c_str(), std::ifstream::binary);
	char buf[300];
	
	int counter = 0;
	if (DEBUG)
		cout<<"Starting read"<<std::endl;
	while (stream)
	{
		stream.read(buf, 256);
		cout<<"buf: "<<buf<<std::endl;
		if ((stream.gcount()) == 256)
		{
			packet * sendPacket = new packet();
			
			sendPacket->endPhoto = CONT_PACKET;
			strncpy(sendPacket->payload, buf, 256);
			dll_send(*sendPacket);
			counter++;
			if (counter > 1)
				DieWithError("Done test");
			//TO DO: Wait on nwl_ACK
		} 
		else 
		{
			cout<<"Read less than 256 bytes."<<std::endl;
			packet * sendPacket = new packet();
			
			//TO DO: Test End of Packet/End of photo
			sendPacket->endPhoto = END_PACKET;
			strncpy(sendPacket->payload, buf, (size_t)(stream.gcount()));
			dll_send(*sendPacket);
			cout<<"SENT A SINGLE PACKET"<<std::endl;
			DieWithError("Done test");
			//TO DO: Wait on new_ACK
		}
		memset(buf, 0, 300);
	}
	cout<<"Reached end of file"<<std::endl;
}

void nwl_recv()																														//Alexi Kessler
{
	
}

void dll_send(packet pkt)																											//Alexi Kessler
{
	bool endPhoto;
	char givenArray[MAX_SIZE_PACKET_PAYLOAD];
	char framePayload[MAX_FRAME_PAYLOAD];
	
	if (pkt.endPhoto == END_PACKET)
	{
		endPhoto = true;
	}
	else 
	{
		endPhoto = false;
	}
	strncpy(givenArray, pkt.payload, MAX_SIZE_PACKET_PAYLOAD);
	
	int i = 0;
	int counter = 0;
	while (i < MAX_SIZE_PACKET_PAYLOAD)
	{
		//if (DEBUG)
			//cout<<"\ni: "<<i<<" counter: "<<counter<<std::endl;
		if (counter < MAX_FRAME_PAYLOAD)
		{
			
			framePayload[counter] = givenArray[i];
			counter++;
		}
		else
		{
			frame* sendFrame = new frame();
			
			sendFrame->seqNumber = sequenceNumber;
			sequenceNumber++;
			sendFrame->frameType = DATA_FRAME;
			if (endPhoto)
				sendFrame->EOP = END_PACKET;
			else 
				sendFrame->EOP = CONT_PACKET;
			strncpy(sendFrame->payload, framePayload, MAX_FRAME_PAYLOAD);
			sendFrame->ED = errorDetectCreate(sendFrame->seqNumber, sendFrame->frameType, sendFrame->EOP, sendFrame->payload);
			phl_send(*sendFrame);
			counter = 0;
			int waitRes = waitEvent();
			if (waitRes == 0)
				cout<<"TIMED OUT. WE TIMED OUT ON A WAIT"<<std::endl;
			//TO DO: Add wait handling from below
		}
		i++;
	}
	
	if (counter!= 0)
	{
		frame* sendFrame = new frame;
			
		sendFrame->seqNumber = sequenceNumber;
		sequenceNumber++;
		sendFrame->frameType = DATA_FRAME;
		if (endPhoto)
			sendFrame->EOP = END_PACKET;
		else 
			sendFrame->EOP = CONT_PACKET;
		strncpy(sendFrame->payload, framePayload, counter+1);
		sendFrame->ED = errorDetectCreate(sendFrame->seqNumber, sendFrame->frameType, sendFrame->EOP, sendFrame->payload);
		phl_send(*sendFrame);
		
		int waitRes = waitEvent();
			if (waitRes == 0)							//Time out
			{
				cout<<"TIMED OUT. WE TIMED OUT ON A WAIT"<<std::endl;
				//Retransmit
				//Start new timer
			}
			else if (waitRes > 0)
			{
				//frame = phl_recv()
				//Check if ack or frame
			}
	}
	//------------------------------------------------------------STILL NEED TO ADD TIMER, WAITING, AND ACK --------------------------------
	//TO DO: For each payload
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

void dll_recv()            																											//Alexi Kessler
{
	
}

int phl_connect(struct sockaddr_in serverAddress, unsigned short serverPort, char *serverName)										//Alexi Kessler
{
	struct addrinfo hints, *servinfo, *p;
	int rv;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; 																									// use AF_INET6 if desire is to force IPv6
	hints.ai_socktype = SOCK_STREAM;

	stringstream tempStream;
	tempStream<<serverPort;
	string tempStr = tempStream.str();
	char * portChar = (char*)tempStr.c_str();
	
	if ((rv = getaddrinfo(serverName, portChar, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		exit(1);
	}
																							
	for(p = servinfo; p != NULL; p = p->ai_next)
	{															
		cout<<"Checking getaddrinfo result"<<std::endl;
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) 
		{
			perror("socket");
			continue;
		}
		memset(&serverAddress, 0, sizeof(serverAddress));  																			//Fill struct with zeroes
		serverAddress.sin_family  = AF_INET;    																					//Set Internet address family
		serverAddress.sin_addr.s_addr =inet_addr(inet_ntoa((struct in_addr) ((struct sockaddr_in *)(p->ai_addr))->sin_addr));  		//Set server IP address using result from getaddrinfo()
		serverAddress.sin_port = htons(serverPort);																					//Set server port. Use well-known port due to getaddrinfo() returning port 80			
		int connectRes;
		if ((connectRes = connect(sockfd, p->ai_addr, p->ai_addrlen)) != 0) 															//Test that connection is successful
		{
			if (DEBUG)
			{
				cout<<"ConnectRes: "<<connectRes<<std::endl;
				cout<<"Error number: "<<strerror(errno)<<std::endl;
			}
			return -1;
		}
		else
			return 0;
	}
	return -2; //Should not reach this point
}

void phl_send(frame fr)																												//Alexi Kessler
{
	cout<<"Physical layer received frame"<<std::endl;
	//printFrame(fr);
	//TO DO: Still need to actually send, this is just for confirmation that it is indeed received.
}

void phl_recv()																														//Alexi Kessler
{
	//recv() wait
}
	
void phl_close()																													//Alexi Kessler
{
	close(sockfd);
}

int waitEvent()																													//Alexi Kessler
{
	int returnVal;
	fd_set readset;
	struct timeval tv;
	
	FD_ZERO (&readset);
	FD_SET (sockfd, &readset);
	
	tv.tv_sec = 2;
	tv.tv_usec = MAX_WAIT_TIME;
	
	returnVal = select(1, &readset, NULL, NULL, &tv);
	
	if (returnVal < 0)
	{
		//TO DO: ERROR HANDLING
	}
	else if (returnVal > 0)
	{
		FD_ZERO (&readset);
	}
	
	return returnVal;
}

short int errorDetectCreate(short int seq, char frameType, char EOP, char payload[])												//Alexi Kessler
{
	return 0;
}

void testSendMessage()																												//Alexi Kessler
{
	int bytesReceived;
	char buffer[100];
	
	cout<<"Testing send message"<<std::endl;
	char * sendChar = "Test message. It worked!!\n";
	int sendLength = strlen(sendChar);
	
	int sendRes = 0;
	sendRes = send(sockfd, sendChar, sendLength, 0);
	if (sendRes != sendLength)
		cout<<"Send failed, different number of bytes sent. Expected: "<<sendLength<<" Sent: "<<sendRes<<std::endl;
	else
		cout<<"Send successful! Sent "<<sendRes<<" bytes!"<<std::endl;
	if ((bytesReceived = recv(sockfd, buffer, 100  - 1, 0)) <= 0)
		DieWithError("Received failed");
	else
		cout<<"Received: "<<buffer<<std::endl;
}

void testSendFrame ()																												//Alexi Kessler
{
	if (DEBUG)
		cout<<"Buidling frame"<<std::endl;
	frame* test = new frame();
	test->seqNumber = 12;
	test->frameType = DATA_FRAME;
	test->EOP = END_PACKET;
	cout<<"Setting payload values"<<std::endl;
	test->payload[0] = '3';
	test->payload[1] = '5';
	test->ED = 12345;
	
	char tempBuf[10];
	char sendChar[MAX_FRAME_SIZE];
	sprintf(sendChar, "%hi%c%c", test->seqNumber, test->frameType, test->EOP);
	int i = 0;
	while ( (test->payload[i] )!='0')
	{
		cout<<"Frame payload["<<i<<"]: "<<test->payload[i]<<std::endl;
		strcat(sendChar, &(test->payload[i]));
		i++;
	}
	
	sprintf(tempBuf, "%hi", test->ED);
	strcat(sendChar, tempBuf);
	
	if (DEBUG)
	{
		cout<<"String to send: "<<sendChar<<std::endl;
	}
	int sendLength = strlen(sendChar);
	
	int sendRes = 0;
	sendRes = send(sockfd, sendChar, sendLength, 0);
	if (sendRes != sendLength)
		cout<<"Send failed, different number of bytes sent. Expected: "<<sendLength<<" Sent: "<<sendRes<<std::endl;
	else
		cout<<"Send successful! Sent "<<sendRes<<" bytes!"<<std::endl;
}

void printFrame (frame fr)																											//Alexi Kessler
{
	cout<<"Sequence Number: "<<fr.seqNumber<<"\nFrame Type: "<<fr.frameType<<"\nEnd of Packet Indicator: "
		<<fr.EOP<<std::endl;
	cout<<"Payload:"<<std::endl;
	int i = 0;
	
	while (i < MAX_FRAME_PAYLOAD)
	{
		cout<<"Frame Payload["<<i<<"]: "<<fr.payload[i]<<" ascii value:"<<(int)fr.payload[i]<<std::endl;
		i++;
	}
	cout<<"Error Detection: "<<fr.ED<<std::endl;
}
