#include <iostream>
#include <unistd.h>
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
#include <sys/stat.h> 
#include <fcntl.h>

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
#define SIZE_RECEIVING_BUFFER 400         																							//TODO: Change this to a more appropriate number
																																	//Sizes for frame parsing
#define FRAME_TYPE_SIZE 1
#define SEQ_NUM_SIZE 5
#define EOP_SIZE 1
#define PAYLOAD_SIZE 130
#define ERRD_SIZE 6
#define MAX_FRAME_PAYLOAD 130
#define USABLE_BYTES 5

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
		unsigned short int dataLength;																								//2 bytes long, used in case payload is not full 130 bytes
		char payload[130];																											//130 bytes long
		short int ED;																										//2 bytes long
};

//typedef enum {frame_arrival} event_type

//Debug variable(s)
bool DEBUG = true;
//Send global var(s)
int sockfd = 0;
short int sequenceNumber = 0;
char recvBuf[SIZE_RECEIVING_BUFFER];
//Logging variables
std::ofstream logFile;
std::string logLoc;
int numPacketsSent = 1;													//Starting value is 1 so incrementing can happen after logging
int numFramesSent = 1;													//Starting value is 1 so incrementing can happen after logging
//Misc global var(s)
fd_set readset;

//Network layer functions
void nwl_read(std::string fileName);
//Data Link layer functions
frame dll_send(packet pkt);
frame dll_recv();
//Physical layer functions
int phl_connect(struct sockaddr_in serverAddress, unsigned short serverPort, char *serverName);
void phl_send(frame fr);
char* phl_recv();
void phl_close();
//Timer functions
void startTimer(int fileDescriptor);
void stopTimer ();
//Misc functions
frame bufToFrame(char * buf);
void DieWithError(char *errorMessage);
short int errorDetectCreate(char* frameInfo, int infoLength);
void testSendMessage();
void testSendFrame();
void printFrame (frame fr);
int waitEvent();
frame makeTestFrame (char frType);																									//Make temporary frame for testing purposes
void testPrintPhoto (std::string loc);
void testWritePacket (packet packet);
frame* read_frame(char* buffer);								//TO DO:Rearrange these categories

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
	if ((argc != 4) || ((strlen(argv[2]))!=4) || ((strlen(argv[3]))!=2))
	{
		cout<<"Proper usage is ./client [hostname] [client id] [number of photos to be read]"<<std::endl;
		cout<<"Client id must be of length 4, number of photos must be of length 2"<<std::endl;
		cout<<"Example: ./client CCCWORK4.WPI.EDU 0023 06"<<std::endl;
		exit(3);
	}
	
	serverName = argv[1];
	cId = atoi(argv[2]);
	numPhoto = atoi(argv[3]);
	
	logLoc = "client_";
	manipStream<<cId<<".log";
	logLoc.append(manipStream.str());
	manipStream.str(std::string());					//Reset manipStream
	logFile.open(logLoc.c_str());
	
	//------------------------------------------------------------------------BEGIN PHOTO READ---------------------------------------------------------------------------
	std::string readLoc; 
	std::string initSendString;
	int count = 0;

	manipStream<<cId;													//Construct initial string to send, containing cId and numPhoto
	initSendString.append(manipStream.str());
	manipStream.str(std::string());												//Reset manipStream
	manipStream<<numPhoto;
	if (numPhoto < 10)
		initSendString.append("0");
	initSendString.append(manipStream.str());
	manipStream.str(std::string());											//Reset manipStream
	
	int connectRes = phl_connect(serverAddress, portNumber, serverName);
	int initSend = 	send(sockfd, initSendString.c_str(), strlen(initSendString.c_str()), 0);
	if (initSend != strlen(initSendString.c_str()))
		DieWithError("Initial send failed");
	
	if (DEBUG)
		cout<<"Phl_connect returned: "<<connectRes<<std::endl;

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
	
	phl_close();																													//Close socket after reading last photo
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
		if (DEBUG)
			cout<<"Read packet, processing"<<std::endl;
		if ((stream.gcount()) == 256)
		{
			packet * sendPacket = new packet();
			
			sendPacket->endPhoto = CONT_PACKET;
			int tempCount = 0;
			while (tempCount<CHUNK_SIZE)
			{
				sendPacket->payload[tempCount] = buf[tempCount];
				tempCount++;
			}
			testWritePacket(*sendPacket);
			frame recvFrame = dll_send(*sendPacket);
			
			//Wait on nwl_ACK (Process recvFrame)
			if (DEBUG)											//Temporary limiter for testing purposes
			{
				counter++;
				if (counter > 2)
					DieWithError("Done test");
			}
		} 
		else 
		{
			cout<<"Read less than 256 bytes."<<std::endl;
			packet * sendPacket = new packet();
			
			sendPacket->endPhoto = END_PACKET;
			strncpy(sendPacket->payload, buf, (size_t)(stream.gcount()));
			frame recvFrame = dll_send(*sendPacket);
			
			logFile<<"Sent packet "<<numPacketsSent<<" to Data Link Layer"<<std::endl;
			numPacketsSent++;
			
			//Wait on nwl_ACK (Process recvFrame)
			if (DEBUG)
			{
				cout<<"SENT A SINGLE PACKET"<<std::endl;
				DieWithError("Done test");
			}
		}
		memset(buf, 0, 300);
	}
	cout<<"Reached end of file"<<std::endl;
}

frame dll_send(packet pkt)																											//Alexi Kessler
{
	bool endPhoto;																													//Values for converting packet to frame																							
	char givenArray[CHUNK_SIZE];
	char framePayload[MAX_FRAME_PAYLOAD];
	int startFrameNumber = numFramesSent;
	
																		//Initial logging
	logFile<<"Sent packet "<<numPacketsSent<<" to Data Link Layer"<<std::endl;
	numPacketsSent++;
	
	if (pkt.endPhoto == END_PACKET)
		endPhoto = true;
	else 
		endPhoto = false;
	int tempCount = 0;
	while (tempCount<CHUNK_SIZE)
	{
		givenArray[tempCount] = pkt.payload[tempCount];
		tempCount++;
	}
	int i = 0;																														//Place in packet payload
	int counter = 0;																												//Place in frame payload
	
	while (i < CHUNK_SIZE)																											//Go byte by byte through the packet
	{
		if (counter < MAX_FRAME_PAYLOAD)
		{
			framePayload[counter] = givenArray[i];																					//Fill frame payload
			counter++;
		}
		else																														//Enough for a frame payload, make into frame and start new frame
		{
			frame* sendFrame = new frame();																																													
			
			sendFrame->seqNumber = sequenceNumber;
			sequenceNumber++;
			sendFrame->frameType = DATA_FRAME;
			if (endPhoto)
				sendFrame->EOP = END_PACKET;																						//TO DO: Revisit this 
			else 
				sendFrame->EOP = CONT_PACKET;
			sendFrame->dataLength = counter;																						//Should be 130
			tempCount = 0;
			while (tempCount<counter)
			{
				sendFrame->payload[tempCount] = framePayload[tempCount];
				tempCount++;
			}
			sendFrame->ED = 0; 																										//Placeholder, actual Error Detect Create takes place right before sending
			phl_send(*sendFrame);																									//Send final frame
			
			numFramesSent++;
			logFile<<"Sent Frame "<<(sendFrame->seqNumber)-startFrameNumber<<" of Packet "<<numPacketsSent<<" to physical layer"<<std::endl;//TO DO: TEST IF THIS AFFECTS TIMEOUT
			
			counter = 0;
			int waitRes = waitEvent();
			if (waitRes == 0)																										//Time out. Need to retransmit
			{
				if (DEBUG)
				cout<<"Timed out while waiting for response"<<std::endl;
				while (waitRes == 0)
				{
					if (DEBUG)
						cout<<"Retransmitting frame: "<<sendFrame->seqNumber<<std::endl;
					phl_send(*sendFrame);
					
					logFile<<"Retransmitted Frame "<<(sendFrame->seqNumber)-startFrameNumber<<" of Packet "<<numPacketsSent
					<<" to physical layer"<<std::endl;//TO DO: TEST IF THIS AFFECTS TIMEOUT
					
					waitRes = waitEvent();
				}
				if (waitRes > 0)
				{
					//TO DO: Add full handling of reply
					if (DEBUG)
						cout<<"Retransmitted sucessfully, received reply"<<std::endl;
				}
			}
			else if (waitRes > 0)																									//Received response before time out. Check if ack
			{
				//Quick hacky test
				char testBuff[260];
				int bytes = 0;
				int z = 0;
				bytes = recv(sockfd, testBuff, 260-1, 0);
				cout<<"Bytes Received:"<<bytes<<std::endl;
				while ((z < bytes) && (testBuff[z] != '\0'))
				{
					cout<<"Received["<<z<<"]: "<<testBuff[z]<<" ascii value: "<<(int)testBuff[z]<<std::endl;
					z++;
				}
				cout<<"Finished receive"<<std::endl;
				//TO DO: Remove when done testing 
				
				//frame* recvFrame = new frame();
				//*recvFrame = dll_recv();
				//Check if ack or frame
					//Check if right ack
			}
		}
		i++;
	}
	if (counter!= 0)																												//Finished parsing packet, some data leftover. Put in frame
	{
		frame* sendFrame = new frame;
			
		sendFrame->seqNumber = sequenceNumber;
		sequenceNumber++;
		sendFrame->frameType = DATA_FRAME;
		sendFrame->EOP = END_PACKET;
		sendFrame->dataLength = counter;																							//Signifies how much leftover data was put into payload
		tempCount = 0;
		while (tempCount<counter)
		{
			sendFrame->payload[tempCount] = framePayload[tempCount];
			tempCount++;
		}
		sendFrame->ED = 0; 																											//Placeholder, actual Error Detect Create takes place right before sending
		phl_send(*sendFrame);
		
		numFramesSent++;
		logFile<<"Sent Frame "<<(sendFrame->seqNumber)-startFrameNumber<<" of Packet "<<numPacketsSent<<" to physical layer"<<std::endl;//TO DO: TEST IF THIS AFFECTS TIMEOUT
		
		int waitRes = waitEvent();
		if (waitRes == 0)							//Time out
		{
			if (DEBUG)
				cout<<"Timed out while waiting for response"<<std::endl;
			while (waitRes == 0)
			{
				if (DEBUG)
					cout<<"Retransmitting frame: "<<sendFrame->seqNumber<<std::endl;
				phl_send(*sendFrame);
				
				logFile<<"Retransmitted Frame "<<(sendFrame->seqNumber)-startFrameNumber<<" of Packet "<<numPacketsSent
					<<" to physical layer"<<std::endl;//TO DO: TEST IF THIS AFFECTS TIMEOUT
					
				waitRes = waitEvent();
			}
			if (waitRes > 0)
			{
				//TO DO: Add full handling of reply
				if (DEBUG)
					cout<<"Retransmitted sucessfully, received reply"<<std::endl;
			}
		}
		else if (waitRes > 0)
		{
			//Quick hacky test
			char testBuff[260];
			int bytes = 0;
			int z = 0;
			bytes = recv(sockfd, testBuff, 260-1, 0);
			cout<<"Bytes Received:"<<bytes<<std::endl;
			while (z < bytes)
			{
				cout<<"Received["<<z<<"]: "<<testBuff[z]<<" ascii value: "<<(int)testBuff[z]<<std::endl;
				z++;
			}
			cout<<"Finished receive"<<std::endl;
			//TO DO: Delete when done testing
			
			//frame* recvFrame = new frame();
			//*recvFrame = dll_recv();
			//Check if ack or frame
				//Check if right ack
		}
	}
	//------------------------------------------------------------STILL NEED TO ADD ACK CHECKING ------------------------------------------
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
	frame * emptyFrame = new frame();																								//This is just for compilaiton purposes. Should never actually return
	return *emptyFrame;
}

frame dll_recv()            																										//Alexi Kessler
{
	frame* returnFrame = new frame();
	char* phlChar;
	phlChar = phl_recv();
	if (DEBUG)
		cout<<"Received: "<<phlChar<<std::endl;
	//returnFrame = bufToFrame(phlChar);					//TO DO: Fill this out
	return *returnFrame;
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
		{
			if (DEBUG)
				cout<<"sockfd: "<<sockfd<<std::endl;
			return 0;
		}
	}
	return -2; 																														//Should not reach this point
}

void phl_send(frame fr)																												//Alexi Kessler
{
	if (DEBUG)
		cout<<"Physical layer received frame:"<<std::endl;
	printFrame(fr);
	
	char tempBuf[10];
	char lengthBuf[10];
	char sendChar[MAX_FRAME_SIZE+40];																								//Char* string to be sent with send()
	int length = 0;
	if (fr.seqNumber < 10)
		sprintf(sendChar, "0000%hi%c%c", fr.seqNumber, fr.frameType, fr.EOP);														//Start concatting everything into sendChar			
	else if (fr.seqNumber < 100)
		sprintf(sendChar, "000%hi%c%c", fr.seqNumber, fr.frameType, fr.EOP);
	else if (fr.seqNumber < 1000)
		sprintf(sendChar, "00%hi%c%c", fr.seqNumber, fr.frameType, fr.EOP);
	else if (fr.seqNumber < 10000)
		sprintf(sendChar, "0%hi%c%c", fr.seqNumber, fr.frameType, fr.EOP);
	else
		sprintf(sendChar, "%hi%c%c", fr.seqNumber, fr.frameType, fr.EOP);
	length += 7;
	
	if (fr.dataLength < 10)
		sprintf(lengthBuf, "0000%hi", fr.dataLength);
	else if (fr.dataLength<100)
		sprintf(lengthBuf, "000%hi", fr.dataLength);
	else if (fr.dataLength<1000)
		sprintf(lengthBuf, "00%hi", fr.dataLength);
	else if (fr.dataLength<10000)
		sprintf(lengthBuf, "0%hi", fr.dataLength);
	else
		sprintf(lengthBuf, "%hi", fr.dataLength);	
	strcat(sendChar, lengthBuf);
	length += 5;
	
	int i = 0;
	while (i<fr.dataLength)
	{
		sendChar[length] = fr.payload[i];
		length++; 
		i++;
	}
	while (i<130)											//Padding
	{
		sendChar[length] = '0';
		length++;
		i++;
	}
	
	if (DEBUG)
		cout<<"Calling errorDetectCreate with length: "<<length<<std::endl;
	fr.ED = errorDetectCreate(sendChar, length);
	if (DEBUG)
		cout<<"Successfully generated ED"<<std::endl;
	if (fr.ED <= -10000)
		sprintf(tempBuf, "%hi", fr.ED);
	else if ((fr.ED<=-1000) && (fr.ED>-10000))
		sprintf(tempBuf, "0%hi", fr.ED);
	else if ((fr.ED<=-100) && (fr.ED>-1000))
		sprintf(tempBuf, "00%hi", fr.ED);
	else if ((fr.ED<=-10) && (fr.ED>-100))
		sprintf(tempBuf, "000%hi", fr.ED);
	else if ((fr.ED<0) && (fr.ED>-10))
		sprintf(tempBuf, "0000%hi", fr.ED);
	else if (fr.ED < 10)
		sprintf(tempBuf, "00000%hi", fr.ED);
	else if (fr.ED < 100)
		sprintf(tempBuf, "0000%hi", fr.ED);
	else if (fr.ED < 1000)
		sprintf(tempBuf, "000%hi", fr.ED);
	else if (fr.ED < 10000)
		sprintf(tempBuf, "00%hi", fr.ED);
	else if (fr.ED < 100000)
		sprintf(tempBuf, "0%hi", fr.ED);
	else 
		DieWithError("Error converting ED to string");
	i = 0;
	while (i<6)
	{
		sendChar[length] = tempBuf[i];
		i++;
		length++;
	}
	
	i = 0;
	while (i<length)
	{
		if (sendChar[i] == 5)
			cout<<"sendChar["<<i<<"]: "<<"Ctrl+E"<<" ascii value:"<<(int)(sendChar[i])<<std::endl;
		else
			cout<<"sendChar["<<i<<"]: "<<sendChar[i]<<" ascii value:"<<(int)(sendChar[i])<<std::endl;
		i++;
	}
	
	int sendRes = 0;
	sendRes = send(sockfd, (void*)sendChar, length, 0);
	if (DEBUG)
	{
		if (sendRes != length)
			cout<<"Send failed, different number of bytes sent. Expected: "<<length<<" Sent: "<<sendRes<<std::endl;
		else
			cout<<"Send successful! Sent "<<sendRes<<" bytes!"<<std::endl;
	}
	if (sendRes != length)
		DieWithError("Failed send");
	
}

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
	
	FD_ZERO (&readset);																												//Zero out readset
	FD_SET (sockfd, &readset);																										//Add sockfd to readset
	
	tv.tv_sec = 1;																													//Set wait time
	tv.tv_usec = MAX_WAIT_TIME;
	
	if (DEBUG)
		cout<<"Starting timeout"<<std::endl;
	returnVal = select(sockfd+1, &readset, NULL, NULL, &tv);																		//Begin monitoring readset
	
	if (returnVal < 0)																												//Something went wrong
	{
		DieWithError("Select encountered fatal error");
	}
	else if (returnVal > 0)																											//Change in socket, ready for recv()
	{
		if (DEBUG)
		{
			cout<<"waitEvent found that socket changed"<<std::endl;
		}
		FD_ZERO (&readset);
	}
	if (DEBUG)
		cout<<"Return value of waitEvent: "<<returnVal<<std::endl;
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
				/*if (DEBUG)
					cout<<"XORING segment starting at "<<counter<<std::endl;*/
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
	retVal = XOR[0] << 8 | XOR[1];			//TO DO: NEED TO TEST ENDIANNESS														//Turn XOR char array into short int
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
		counter++;
	}
	cout<<"Set payload values"<<std::endl;
	test->ED = 2391;																												//Placeholder, actual value added before send
	
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
		if (fr.payload[i] == 5)
			cout<<"Frame Payload["<<i<<"]: "<<"Ctrl+E"<<" ascii value:"<<(int)fr.payload[i]<<std::endl;
		else
			cout<<"Frame Payload["<<i<<"]: "<<fr.payload[i]<<" ascii value:"<<(int)fr.payload[i]<<std::endl;
		i++;
	}
	cout<<"Error Detection: "<<fr.ED<<std::endl;
}

frame makeTestFrame (char frType)																									//Alexi Kessler
{
	frame* fr = new frame();
	fr->seqNumber = 12;
	fr->frameType = frType;
	fr->EOP = END_PACKET;
	fr->dataLength = 130;
	cout<<"Setting payload values"<<std::endl;
	int counter = 0;
	while (counter<fr->dataLength)
	{
		fr->payload[counter] = '9';
		counter++;
	}
	cout<<"Set payload values"<<std::endl;
	fr->ED = 2391;																													//Placeholder, actual value added before send
	return *fr;
}

void testPrintPhoto (std::string loc)																								//Alexi Kessler
{
	FILE *file = fopen(loc.c_str(), "r");
    char code[1000];
    size_t n = 0;
    int c;

    if (file == NULL)
		cout<<"Could not open"<<std::endl;

    while (((c = fgetc(file)) != EOF) && (c<230))
    {
        code[n++] = (char) c;
    }
	n = 0;
	while (n<230)
	{
		cout<<"Char "<<n<<": "<<code[n]<<std::endl;
		n++;
	}
}

void testWritePacket(packet packet)																									//Alexi Kessler
{
	std::ofstream outfile ("new.txt",std::ofstream::binary);
	outfile.write (packet.payload, 256);
}

frame* read_frame(char* buffer){																									//Juan Rodriguez
	frame* new_frame = new frame();
	
	int i = 0;
	int place = 0;
	int startFrameType = SEQ_NUM_SIZE;
	int startEOP = startFrameType + FRAME_TYPE_SIZE;
	int startUsableBytes = startEOP + EOP_SIZE;
	int startPayload = startUsableBytes + USABLE_BYTES;
	int startERRD = startPayload + PAYLOAD_SIZE;
	char seq_num[SEQ_NUM_SIZE + 1];
	char frameType;
	char usable_b[USABLE_BYTES + 1];
	char ED[ERRD_SIZE + 1];
	char payload[130];
		
	memset(seq_num, 0, strlen(seq_num));
	
	cout<<"Parsing sequence number...\n";
	while (i < place+SEQ_NUM_SIZE){
		
		if(DEBUG)
		{
			cout<<"Value of i: "<<i<<" Value of place: "<<place<<std::endl;
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
	while (i< place+FRAME_TYPE_SIZE)
	{
		strncat(&new_frame->frameType, &buffer[i], 1);
		i++;
	}
	cout<<"Parsed frame type: "<<frameType<<std::endl;
	
	if(DEBUG)
		cout<<"Value i: "<<i<<"\nPlace: "<<place<<std::endl;
	
	place=i;
	
	cout<<"Parsing EOP...\n";
	while (i < place+EOP_SIZE)
	{	
		if(DEBUG)
			cout<<"EOP while 'i' value: "<<i<<std::endl;
		
		strncpy(&new_frame->EOP, &buffer[i], 1);
		i++;
	}
	cout<<"Parsed EOP...\n";
	place =i;
	
	if(DEBUG)
		cout<<"Value i: "<<i<<"\nPlace: "<<place<<std::endl;

	cout<<"Parsing usable bytes...\n";
	while (i < place+USABLE_BYTES)
	{
		if(DEBUG)
			cout<<"EOP while 'i' value: "<<i<<std::endl;
		
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

	while (i < place+PAYLOAD_SIZE)
	{
		if(DEBUG)
			cout<<"Value i: "<<i<<std::endl;
	
		strncpy(&new_frame->payload[i-startPayload], &buffer[i], 1);
		i++;
		
		if(DEBUG)
		{
			cout<<"Value i: "<<i<<std::endl;
			cout<<"Payload["<<i-startPayload<<"] "<<&payload[i-startPayload]<<std::endl;
		}
	}
	//memset(&new_frame->payload[bytes_rcvd], 0, PAYLOAD_SIZE - bytes_rcvd);
	cout<<"Parsed payload...\n";
	
	place=i;
	
	if(DEBUG)
		cout<<"Value i: "<<i<<"\nPlace: "<<place<<std::endl;
	
	cout<<"Parsing Error Detection...\n";
	while(i < place+ERRD_SIZE)
	{
		strncpy(&ED[i-startERRD], &buffer[i], 1);
		i++;
	}
	cout<<"Error Detection string: "<<ED<<std::endl;
	ED[ERRD_SIZE] = '\0';

	int parse = 0;
	int ignore = 0;
	while(parse < ERRD_SIZE)
	{ 
		if(ED[parse] == '0')
		{
			ignore++;
		}
		else if(ED[parse] == '-')
		{
			break;
		}
		parse++;
	}
	
	if(DEBUG)
	{
		cout<<"Ignore: "<<ignore<<std::endl;
		cout<<"Parsed Error Detection Value is: "<<ED<<std::endl;
	}
	new_frame->seqNumber = atoi(seq_num);
	/***********************************NEEDS TO BE CHECKED****************************/
	//new_frame->payload[PAYLOAD_SIZE] = '\0';
	
	new_frame->dataLength = bytes_rcvd;
	new_frame->ED = atoi(&ED[ignore]);
	
	printFrame(*new_frame);
	return new_frame;	
}

