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
#define CHUNK_SIZE 256
#define PACKET_SIZE CHUNK_SIZE+1
#define MAX_FRAME_PAYLOAD 130
#define MAX_FRAME_SIZE 138
#define DATA_FRAME 'D'
#define ACK_FRAME 'A'
#define END_PACKET 'E'
#define CONT_PACKET 'C'
#define MAX_WAIT_TIME 750
#define SIZE_RECEIVING_BUFFER 400         //TODO: Change this to a more appropriate number

using namespace std;

class packet																														//Alexi Kessler
{
	public:
		char payload[256]; 																											//256 bytes long
		char endPhoto;																												//1 byte long end of photo indicator
};

class frame																															//Alexi Kessler
{
	public:
		short int seqNumber; 																										//2 bytes long
		char frameType; 																											//1 byte long
		char EOP;																													//1 byte long
		short int dataLength;																										//2 bytes long, used in case payload is not full 130 bytes
		char payload[130];																											//130 bytes long
		short int ED;																												//2 bytes long
};

//typedef enum {frame_arrival} event_type

//Debug variable(s)
bool DEBUG = true;
//Send global var(s)
int sockfd = 0;
short int sequenceNumber = 0;
char recvBuf[SIZE_RECEIVING_BUFFER];
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
char* phl_recv();
void phl_close();
//Misc functions
frame bufToFrame(char * buf);
void DieWithError(char *errorMessage);
short int errorDetectCreate(char* frameInfo, int infoLength);
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
	if (argc != 4)
	{
		cout<<"Proper usage is ./client [hostname] [client id] [number of photos to be read]"<<std::endl;
		exit(3);
	} 
	
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
	{																																//Set new readLoc
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
	char currChar;
	
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
			if (counter > 0)
				DieWithError("Done test");
			//Wait on nwl_ACK
		} 
		else 
		{
			cout<<"Read less than 256 bytes."<<std::endl;
			packet * sendPacket = new packet();
			
			sendPacket->endPhoto = END_PACKET;
			strncpy(sendPacket->payload, buf, (size_t)(stream.gcount()));
			dll_send(*sendPacket);
			cout<<"SENT A SINGLE PACKET"<<std::endl;
			DieWithError("Done test");
			//Wait on new_ACK
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
	char givenArray[CHUNK_SIZE];
	char framePayload[MAX_FRAME_PAYLOAD];
	if (pkt.endPhoto == END_PACKET)
	{
		endPhoto = true;
	}
	else 
	{
		endPhoto = false;
	}
	strncpy(givenArray, pkt.payload, CHUNK_SIZE);
	int i = 0;
	int counter = 0;
	
	while (i < CHUNK_SIZE)
	{
		if (DEBUG)
			cout<<"\ni: "<<i<<" counter: "<<counter<<std::endl;
		if (counter < MAX_FRAME_PAYLOAD)
		{
			if (DEBUG)
				cout<<"Copying "<<i<<" character of array"<<std::endl;
			framePayload[counter] = givenArray[i];
			counter++;
		}
		else																														//Filled a frame, send it along and start new frame
		{
			frame* sendFrame = new frame();
			
			sendFrame->seqNumber = sequenceNumber;
			sequenceNumber++;
			sendFrame->frameType = DATA_FRAME;
			if (endPhoto)
				sendFrame->EOP = END_PACKET;
			else 
				sendFrame->EOP = CONT_PACKET;
			sendFrame->dataLength = counter;																						//Should be 130
			strncpy(sendFrame->payload, framePayload, MAX_FRAME_PAYLOAD);
			sendFrame->ED = 0; 																										//Placeholder, actual Error Detect Create takes place right before sending
			phl_send(*sendFrame);
			counter = 0;
			int waitRes = waitEvent();
			if (waitRes == 0)
				cout<<"TIMED OUT. WE TIMED OUT ON A WAIT"<<std::endl;
			//TO DO: Add wait handling from below
		}
		i++;
	}
	
	if (counter!= 0)																												//Finished parsing packet, some data leftover. Put in frame
	{
		frame* sendFrame = new frame;
			
		sendFrame->seqNumber = sequenceNumber;
		sequenceNumber++;
		sendFrame->frameType = DATA_FRAME;
		if (endPhoto)
			sendFrame->EOP = END_PACKET;
		else 
			sendFrame->EOP = CONT_PACKET;
		sendFrame->dataLength = counter;																							//Signifies how much leftover data was put into payload
		strncpy(sendFrame->payload, framePayload, counter+1);
		sendFrame->ED = 0; 																											//Placeholder, actual Error Detect Create takes place right before sending
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
	//bufToFrame
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
	{																																//loop through all the results 		
		
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
		if ((connectRes = connect(sockfd, p->ai_addr, p->ai_addrlen)) != 0) 														//Test that connection is successful
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
	return -2; 																														//Should not reach this point
}

void phl_send(frame fr)																												//Alexi Kessler
{
	cout<<"Physical layer received frame:"<<std::endl;
	printFrame(fr);
	
	char tempBuf[10];																										
	char sendChar[MAX_FRAME_SIZE];																									//Char* string to be sent with send()
	sprintf(sendChar, "%hi%c%c%hi", fr.seqNumber, fr.frameType, fr.EOP, fr.dataLength);												//Start concatting everything into sendChar			
	int i = 0;
	while (i<fr.dataLength)
	{
		cout<<"Frame payload["<<i<<"]: "<<fr.payload[i]<<std::endl;
		strcat(sendChar, &(fr.payload[i]));
		i++;
	}
	
	fr.ED = errorDetectCreate(sendChar, strlen(sendChar));
	
	sprintf(tempBuf, "%hi", fr.ED);
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
}	//TO DO: Test this 

char* phl_recv()																													//Alexi Kessler
{
	int bytesReceived;									
	
	bytesReceived = recv(sockfd, recvBuf, SIZE_RECEIVING_BUFFER-1, 0);
	if (bytesReceived <= 0)
		DieWithError("Issue with phl_recv");
	return recvBuf;	
}
	
void phl_close()																													//Alexi Kessler
{
	close(sockfd);
}

int waitEvent()																														//Alexi Kessler
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

short int errorDetectCreate(char* info, int infoLength)																				//Alexi Kessler
{
	char frameInfo[MAX_FRAME_SIZE];
	strncpy(frameInfo, info, infoLength+1);
	int counter = 0;
	char tempArray1[10];  																											//Honestly, I don't know why these need more than two bytes. But
	char tempArray2[10];																											//when allocating only two, they kept overwriting the counter
	char XOR[1];																													//Two bytes
	char tempChar1;
	char tempChar2;
	short int retVal;
	while (counter < infoLength)
	{
		if (DEBUG)
			cout<<"ErrorDetectCreate cycle with counter: "<<counter<<std::endl;
		if (counter == 0)
		{
			/*cout<<"Before first copy, frameInfo[0]: "<<frameInfo[0]<<" frameInfo[1]: "<<frameInfo[1]<<std::endl;
			cout<<"tempArray1[0]: "<<tempArray1[0]<<" tempArray1[1]: "<<tempArray1[1]<<std::endl;
			cout<<"counter: "<<counter<<std::endl;
			cout<<"tempArray1 address"<<&tempArray1<<std::endl;
			cout<<"frameInfo address"<<&frameInfo<<std::endl;
			cout<<"counter address"<<&counter<<std::endl;*/
			strncpy(tempArray1, &frameInfo[0], 2);
			strncpy(tempArray2, &frameInfo[counter + 2], 2);
			tempChar1 = (char)(tempArray1[0]^tempArray2[0]);
			tempChar2 = (char)(tempArray1[1]^tempArray2[1]);
			strncpy(&XOR[counter],  &tempChar1,  1);
			strncpy(&XOR[counter+1], &tempChar2, 1);
			cout<<"At end of initial fold, counter = "<<counter<<" and XOR = "<<XOR[counter]<<XOR[counter+1]<<std::endl;
			counter+=2;
		}
		else																													
		{
			if ((infoLength-counter) == 0) 	//NEEDS TESTING																				//Odd number length and at end 
			{
				if (DEBUG)
					cout<<"XORing last segment"<<std::endl;
				strncpy(tempArray1, &XOR[0], 2);
				strncpy(tempArray2, &frameInfo[counter],  1);
				tempArray2[1] = 0;
				tempChar1 = (char)(tempArray1[0]^tempArray2[0]);
				tempChar2 = (char)(tempArray1[1]^tempArray2[1]);
				strncpy(&XOR[0], &tempChar1, 1);
				strncpy(&XOR[1], &tempChar2, 1);
				counter+=2;
			}
			else
			{
				int tracker = counter;
				if (DEBUG)
					cout<<"XORING segment starting at "<<counter<<std::endl;
				strncpy(tempArray1, &XOR[0], 2);
				strncpy(tempArray2, &frameInfo[counter],  2);
				tempChar1 = (char)(tempArray1[0]^tempArray2[0]);
				tempChar2 = (char)(tempArray1[1]^tempArray2[1]);
				strncpy(&XOR[0], &tempChar1, 1);
				strncpy(&XOR[1], &tempChar2, 1);
				counter+=2;
				if (counter == tracker)
					DieWithError("Broken ED create");
			}
		}
	}
	retVal = XOR[0] << 8 | XOR[1];			//NEED TO TEST ENDIANNESS																//Turn COR char array into short int
	if (DEBUG)
	{
		cout<<"Created XOR value of: "<<retVal<<std::endl;
	}
	return retVal;
}

void testSendMessage()																												//Alexi Kessler
{
	cout<<"Testing send message"<<std::endl;
	char * sendChar = "Test message. It worked!!\n";
	int sendLength = strlen(sendChar);
	
	int sendRes = 0;
	sendRes = send(sockfd, sendChar, sendLength, 0);
	if (sendRes != sendLength)
		cout<<"Send failed, different number of bytes sent. Expected: "<<sendLength<<" Sent: "<<sendRes<<std::endl;
	else
		cout<<"Send successful! Sent "<<sendRes<<" bytes!"<<std::endl;
}

void testSendFrame ()																												//Alexi Kessler
{
	if (DEBUG)
		cout<<"Buidling frame"<<std::endl;
	frame* test = new frame();
	test->seqNumber = 12;
	test->frameType = DATA_FRAME;
	test->EOP = END_PACKET;
	test->dataLength = 130;
	
	cout<<"Setting payload values"<<std::endl;
	int counter = 0;
	while (counter<test->dataLength)
	{
		test->payload[counter] = '9';
		//cout<<"Set payload["<<counter<<"] to "<<test->payload[counter]<<std::endl;
		counter++;
	}
	cout<<"Set payload values"<<std::endl;
	
	char tempBuf[10];
	char sendChar[MAX_FRAME_SIZE*10];
	char tempChar[1];
	sprintf(sendChar, "%hi%c%c%hi", test->seqNumber, test->frameType, test->EOP, test->dataLength);
	
	int i = 0;
	while (i < (test->dataLength))
	{
		tempChar[0] = test->payload[i];
		tempChar[1] = '\0';
		strcat(sendChar,  tempChar);
	
		i+=1;
	}
	
	if (DEBUG)
	{
		cout<<"String to send before adding ED: "<<sendChar<<std::endl;
	}
	
	cout<<"Calling errorDetectCreate with sendChar and length: "<<strlen(sendChar)<<std::endl;
	test->ED = errorDetectCreate(sendChar, strlen(sendChar));
	
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
		cout<<"Frame Payload["<<i<<"]: "<<fr.payload[i]<<" acii value:"<<(int)fr.payload[i]<<std::endl;
		i++;
	}
	cout<<"Error Detection: "<<fr.ED<<std::endl;
}
