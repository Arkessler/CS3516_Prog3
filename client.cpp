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
#include <sys/time.h>

#define DEFAULT_PORT 1199
#define CHUNK_SIZE 256
#define PACKET_SIZE CHUNK_SIZE+1
#define MAX_FRAME_PAYLOAD 130
#define MAX_FRAME_SIZE 138
#define DATA_FRAME 'D'
#define ACK_FRAME 'A'
#define END_PACKET 'E'
#define CONT_PACKET 'C'
#define END_PHOTO 'Y'
#define CONT_PHOTO 'N'
#define MAX_WAIT_TIME 750
#define SIZE_RECEIVING_BUFFER 150         																							//TODO: Change this to a more appropriate number
																																	//Sizes for frame parsing
#define FRAME_TYPE_SIZE 1
#define SEQ_NUM_SIZE 5
#define EOP_SIZE 1
#define PAYLOAD_SIZE 130
#define ERRD_SIZE 6
#define MAX_FRAME_PAYLOAD 130
#define USABLE_BYTES 5

#define NUM_TEST_PACKETS 40
using namespace std;

/*-----------------------------------------Functionalities to add---------------------
Logging of all events
Running tally of significant events
Introduce frame transmission error
*/

class packet																														//Alexi Kessler
{
	public:
		char payload[256]; 																											//256 bytes long
		char endPhoto;																												//1 byte long end of photo indicator
		unsigned short int dataLength;
};

class frame																															//Alexi Kessler
{
	public:
		short int seqNumber; 																										//2 bytes long
		char frameType; 																											//1 byte long
		char EOP;																													//1 byte long
		unsigned short int dataLength;																								//2 bytes long, used in case payload is not full 130 bytes
		char payload[130];																											//130 bytes long
		short int ED;																												//2 bytes long
		char endPhoto;
};

//Debug variable(s)
bool DEBUG = false;
//Send global var(s)
int sockfd = 0;
short int sequenceNumber = 1;
char recvBuf[SIZE_RECEIVING_BUFFER];
//Logging variables
std::ofstream logFile;
std::string logLoc;
int numPacketsSent = 0;																												//Starting value is 0 becauase logging happens after incrementation
int numFramesSent = 1;																												//Starting value is 1 so incrementing can happen after logging
int numFramesRetransmitted = 0;
int totalGoodAcksReceived = 0;
int totalBadAcksReceived = 0;
struct timeval startTime;
struct timeval endTime;
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
void DieWithError(char *errorMessage);
short int errorDetectCreate(char* frameInfo, int infoLength);
void testSendMessage();
void testSendFrame();
void printFrame (frame fr);
int waitEvent();
frame makeTestFrame (char frType);																									//Make temporary frame for testing purposes
void testPrintPhoto (std::string loc);
void testWritePacket (packet packet);
frame* stringToFrame(char* buffer);																									

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
	manipStream.str(std::string());																									//Reset manipStream
	logFile.open(logLoc.c_str());
	
	//------------------------------------------------------------------------BEGIN PHOTO READ---------------------------------------------------------------------------
	std::string readLoc; 
	std::string initSendString;
	int count = 0;

	manipStream<<cId;																												//Construct initial string to send, containing cId and numPhoto
	initSendString.append(manipStream.str());
	manipStream.str(std::string());																									//Reset manipStream
	manipStream<<numPhoto;
	if (numPhoto < 10)
		initSendString.append("0");																									//Pad so that result is 6 characters long
	initSendString.append(manipStream.str());
	manipStream.str(std::string());																									//Reset manipStream
	
	int connectRes = phl_connect(serverAddress, portNumber, serverName);															//Establish connection to server
	int initSend = 	send(sockfd, initSendString.c_str(), strlen(initSendString.c_str()), 0);										//Send initial message
	if (initSend != (int)strlen(initSendString.c_str()))
		DieWithError("Initial send failed");
	
	if (DEBUG)
		cout<<"Phl_connect returned: "<<connectRes<<std::endl;

	int getTimeRes = gettimeofday(&startTime, NULL);
	for (count = 1; count<(numPhoto+1); count++)
	{																																//Set new readLoc
		readLoc = "photo";
		manipStream<<cId;																											//Convert cId to string
		readLoc.append(manipStream.str());
		manipStream.str(std::string());																								//Reset manipStream
		manipStream<<count;																											//Convert count to string
		readLoc.append(manipStream.str());   																						
		manipStream.str(std::string());																								//Reset manipStream
		readLoc.append(".jpg");
		if (DEBUG)
			cout<< "New read location: "<<readLoc<<std::endl;
		nwl_read(readLoc);																											//Have network layer read file
		logFile<<"\n----------------Finished Sending Photo"<<count<<"-----------------------------"<<std::endl;
	}
	getTimeRes = gettimeofday(&endTime, NULL);
	int executeTime = (endTime.tv_sec - startTime.tv_sec);
	logFile<<"Total time to transmit all photos: "<<executeTime<<" seconds"<<std::endl;
	logFile<<"Total number of frames sent: "<<numFramesSent<<std::endl;
	logFile<<"Number of frames that were retransmission: "<<numFramesRetransmitted<<std::endl;
	logFile<<"Total number of good ACK's received: "<<totalGoodAcksReceived<<std::endl;
	logFile<<"Total number of bad ACK's received: "<<totalBadAcksReceived<<std::endl;
	phl_close();																													//Close socket after reading last photo
}

void nwl_read(std::string fileName)																									//Alexi Kessler
{
	std::ifstream stream (fileName.c_str(), std::ifstream::binary);
	char buf[300];
	int counter = 0;
	if (DEBUG)
		cout<<"Starting read"<<std::endl;
	while (stream)																													//While there is something left to read 
	{
		int tempCount = 0;
		stream.read(buf, 256);
		if (DEBUG)
			cout<<"Read packet, processing"<<std::endl;
		if ((stream.gcount()) == 256)																								//If full read of 256 bytes
		{
			packet * sendPacket = new packet();																						//Put read data into new packet
			
			sendPacket->endPhoto = CONT_PHOTO;
			sendPacket->dataLength = 256;
			tempCount = 0;
			while (tempCount<CHUNK_SIZE)																			
			{
				sendPacket->payload[tempCount] = buf[tempCount];
				tempCount++;
			}
			testWritePacket(*sendPacket);																
			frame recvFrame = dll_send(*sendPacket);																				//Send packet to dll and get result
			
			//Wait on nwl_ACK (Process recvFrame)
			if (DEBUG)																												//Temporary limiter for testing purposes
			{
				counter++;
				if (counter > NUM_TEST_PACKETS)
					DieWithError("Done test");
			}
		} 
		else 																														//Read less than 256 bytes, signifying end of photo															
		{
			if (DEBUG)
				cout<<"Read less than 256 bytes."<<std::endl;
			packet * sendPacket = new packet();
			
			sendPacket->endPhoto = END_PHOTO;
			sendPacket->dataLength = (stream.gcount());
			
			tempCount = 0;
			while (tempCount<(stream.gcount()))																			
			{
				sendPacket->payload[tempCount] = buf[tempCount];
				tempCount++;
			}
			frame recvFrame = dll_send(*sendPacket);
			
			//Wait on nwl_ACK (Process recvFrame)
			if (DEBUG)
			{
				cout<<"Sent final packet of photo"<<std::endl;
				DieWithError("Done test");
			}
		}
		memset(buf, 0, 300);																										//Clear read buffer
	}
	cout<<"Reached end of file"<<std::endl;
}

frame dll_send(packet pkt)																											//Alexi Kessler
{
	bool endPhoto;																													//Values for converting packet to frame																							
	char givenArray[CHUNK_SIZE];
	char framePayload[MAX_FRAME_PAYLOAD];
	int startFrameNumber = numFramesSent-1-numFramesRetransmitted;
	unsigned short int lengthPacket = pkt.dataLength;
	bool retransmit = false;
	
	numPacketsSent++;	
	logFile<<"\nSent packet "<<numPacketsSent<<" to Data Link Layer"<<std::endl;														//Initial logging. Placed here to ensure success of packet send.
	cout<<"Sent packet "<<numPacketsSent<<" to Data Link Layer"<<std::endl;
	if (pkt.endPhoto == END_PHOTO)
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
			if (counter == lengthPacket)
				sendFrame->EOP = END_PACKET;																						
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
			if (endPhoto && ((pkt.dataLength) == 130))
			{
				cout<<"pkt datalength: "<<pkt.dataLength<<std::endl;
				sendFrame->endPhoto = END_PHOTO;
			}
			else
				sendFrame->endPhoto = CONT_PHOTO;
			
			do																														//While retransmit
			{
				phl_send(*sendFrame);																								//Send final frame
			
				numFramesSent++;
				logFile<<"Sent Frame "<<(sendFrame->seqNumber)-startFrameNumber<<" of Packet "
					<<numPacketsSent<<" to physical layer"<<std::endl;																
				
				counter = 0;
				int waitRes = waitEvent();
				if (waitRes == 0)																									//Time out. Need to retransmit
				{																													
					if (DEBUG)
					cout<<"Timed out while waiting for response"<<std::endl;
					while (waitRes == 0)
					{
						logFile<<"Timed out waiting for ACK on Frame "<<sendFrame->seqNumber
							<<" of Packet "<<numPacketsSent<<std::endl;
						if (DEBUG)
							cout<<"Retransmitting frame: "<<sendFrame->seqNumber<<std::endl;
						phl_send(*sendFrame);
						numFramesSent++;
						numFramesRetransmitted++;
						
						logFile<<"Retransmitted Frame "<<(sendFrame->seqNumber)<<" of Packet "<<numPacketsSent
						<<" to physical layer"<<std::endl;																			
						
						waitRes = waitEvent();
					}	
					if (waitRes > 0)											
					{
						if (DEBUG)
							cout<<"Receiving frame"<<std::endl;
						frame* recvFrame = new frame();
						*recvFrame = dll_recv();
						if (recvFrame->frameType == ACK_FRAME)
						{
							if ((recvFrame->seqNumber == sendFrame->seqNumber) &&
								(recvFrame->seqNumber == recvFrame->ED))															//If right ack number and not corrupted
							{
								totalGoodAcksReceived++;
								logFile<<"Received valid ACK frame for sentFrame "
									<<sendFrame->seqNumber<<" belonging to Packet "<<numPacketsSent<<std::endl;
								if (DEBUG)
									cout<<"Received valid ACK reply"<<std::endl;
								retransmit = false;
							}
							else
							{
								totalBadAcksReceived++;
								logFile<<"Received bad ACK frame"<<std::endl;
								if (!(recvFrame->seqNumber == sendFrame->seqNumber))												//If wrong number ACK
								{
									logFile<<"ACK Sequence Number: "<<recvFrame->seqNumber
										<<" Sent Frame Sequence Number: "<<sendFrame->seqNumber<<std::endl;
								}
								else if (!(recvFrame->seqNumber != recvFrame->ED))													//If ACK corrupted
								{
									logFile<<"ACK ED: "<<recvFrame->ED<<" ACK seqNumber: "<<recvFrame->seqNumber<<std::endl;
								}
								retransmit = true;
							} 
						}
						else
						{
							printFrame(*recvFrame);
							DieWithError("Received frame is invalid");
						}
						if (DEBUG)
							cout<<"Retransmitted sucessfully, received reply"<<std::endl;
					}
				}
				else if (waitRes > 0)																								//Received response before time out. Check if ack
				{
					frame* recvFrame = new frame();
					*recvFrame = dll_recv();
					if (DEBUG)
					{
						cout<<"Frame seq: "<<recvFrame->seqNumber;
						cout<<"sendFrame seq: "<<sendFrame->seqNumber;
						cout<<"Frame ED: "<<recvFrame->ED;
					}
					if (recvFrame->frameType == ACK_FRAME)
					{
						if ((recvFrame->seqNumber == sendFrame->seqNumber) &&
							(recvFrame->seqNumber == recvFrame->ED))																//If right ack number and not corrupted
						{
							totalGoodAcksReceived++;
							logFile<<"Received valid ACK frame for sentFrame "
									<<sendFrame->seqNumber<<" belonging to Packet "<<numPacketsSent<<std::endl;
							if (DEBUG)
								cout<<"Received valid ACK reply"<<std::endl;
							retransmit = false;
						}
						else
						{
							totalBadAcksReceived++;
							logFile<<"Received bad ACK frame"<<std::endl;
							if (!(recvFrame->seqNumber == sendFrame->seqNumber))													//If wrong number ACK
							{
								logFile<<"ACK Sequence Number: "<<recvFrame->seqNumber
									<<" Sent Frame Sequence Number: "<<sendFrame->seqNumber<<std::endl;
							}
							else if (!(recvFrame->seqNumber == recvFrame->ED))														//If ACK corrupted
							{
								logFile<<"ACK ED: "<<recvFrame->ED<<" ACK seqNumber: "<<recvFrame->seqNumber<<std::endl;
							}
							retransmit = true;																						//Fall through and send again
						} 
					}
					else
					{
						printFrame(*recvFrame);
						DieWithError("Received frame is invalid");
					}
				}																													//end ACK receive 
			} while (retransmit);																									//end frame transmission
		} 																															//end frame construction
		i++;
	}
	//----------------------------------------------------------------------------End looping through packet-----------------------------------------------------------------------------------
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
		if (endPhoto)
			sendFrame->endPhoto = END_PHOTO;
		else
			sendFrame->endPhoto = CONT_PHOTO;
		do																															//While retransmit
		{
			phl_send(*sendFrame);
			
			numFramesSent++;
			logFile<<"Sent Frame "<<(sendFrame->seqNumber)-startFrameNumber<<" of Packet "
				<<numPacketsSent<<" to physical layer"<<std::endl;																	
			
			int waitRes = waitEvent();
			if (waitRes == 0)																										//In the case of a time out
			{																														
				logFile<<"Timed out waiting for ACK on Frame "<<sendFrame->seqNumber
							<<" of Packet "<<numPacketsSent<<std::endl;
				if (DEBUG)
					cout<<"Timed out while waiting for response"<<std::endl;
				while (waitRes == 0)
				{
					if (DEBUG)
						cout<<"Retransmitting frame: "<<sendFrame->seqNumber<<std::endl;	
					phl_send(*sendFrame);																							//Retransmit frame
					
					numFramesSent++;
					numFramesRetransmitted++;
					
					logFile<<"Retransmitted Frame "<<(sendFrame->seqNumber)-startFrameNumber<<" of Packet "<<numPacketsSent
						<<" to physical layer"<<std::endl;																			
						
					waitRes = waitEvent();																							//Start new timer
				}
				if (waitRes > 0)																									//If ready to receive a reply																			
				{
					frame* recvFrame = new frame();
					*recvFrame = dll_recv(); 
					if (recvFrame->frameType == ACK_FRAME)
					{
						if (recvFrame->frameType == ACK_FRAME)
						{
							if ((recvFrame->seqNumber == sendFrame->seqNumber) &&	
								(recvFrame->seqNumber == recvFrame->ED))															//If right ack number and not corrupted
							{
								totalGoodAcksReceived++;
								logFile<<"Received valid ACK frame for sentFrame "
									<<sendFrame->seqNumber<<" of Packet "<<numPacketsSent<<std::endl;
								if (DEBUG)
									cout<<"Received valid ACK reply"<<std::endl;
								retransmit = false;
							}
							else
							{
								totalBadAcksReceived++;
								logFile<<"Received bad ACK frame"<<std::endl;
								if (!(recvFrame->seqNumber == sendFrame->seqNumber))												//If wrong number ack
								{
									logFile<<"ACK Sequence Number: "<<recvFrame->seqNumber
										<<" Sent Frame Sequence Number: "<<sendFrame->seqNumber<<std::endl;
								}
								else if (!(recvFrame->seqNumber == recvFrame->ED))													//If ACK corrupted
								{
									logFile<<"ACK ED: "<<recvFrame->ED<<" ACK seqNumber: "<<recvFrame->seqNumber<<std::endl;
								}
								retransmit = true;
							} 
						}
					}
					else
					{
						printFrame(*recvFrame);
						DieWithError("Received frame is invalid");
					}
					if (DEBUG)
						cout<<"Retransmitted sucessfully, received reply"<<std::endl;
				}
			}
			else if (waitRes > 0)
			{
				frame* recvFrame = new frame();
				*recvFrame = dll_recv();
				if (recvFrame->frameType == ACK_FRAME)																					
				{
					if (recvFrame->frameType == ACK_FRAME)
					{
						if ((recvFrame->seqNumber == sendFrame->seqNumber) &&
							(recvFrame->seqNumber == recvFrame->ED))																	//If right ack number and not corrupted
						{
							totalGoodAcksReceived++;
							logFile<<"Received valid ACK frame for sentFrame "
									<<sendFrame->seqNumber<<" of Packet "<<numPacketsSent<<std::endl;
							if (DEBUG)
								cout<<"Received valid ACK reply"<<std::endl;
							retransmit = false;
						}
						else
						{
							totalBadAcksReceived++;
							logFile<<"Received bad ACK frame"<<std::endl;
							if (!(recvFrame->seqNumber == sendFrame->seqNumber))													//If wrong number ACK
							{
								logFile<<"ACK Sequence Number: "<<recvFrame->seqNumber
									<<" Sent Frame Sequence Number: "<<sendFrame->seqNumber<<std::endl;
							}
							else if (!(recvFrame->seqNumber == recvFrame->ED))														//If ACK corrupted
							{
								logFile<<"ACK ED: "<<recvFrame->ED<<" ACK seqNumber: "<<recvFrame->seqNumber<<std::endl;
							}
							retransmit = true;
						} 
					}
				}
				else
				{
					printFrame(*recvFrame);
					DieWithError("Received frame is invalid");
				}
			}																														//end ACK receive 									
		} while (retransmit);																										//end frame transmission
	}																																//end frame construction
	frame* recvFrame = new frame();
	*recvFrame = dll_recv();
	if (recvFrame->frameType == DATA_FRAME)
	{
		logFile<<"Received nwl_ACK for Packet "<<numPacketsSent<<std::endl;
		return *recvFrame;
	}
	else
	{
		DieWithError("Received DLL_ACK when expecting NWL_ACK");
	}
	
	frame * emptyFrame = new frame();																								//This is just for compilaiton purposes. Should never actually return
	return *emptyFrame;
}

frame dll_recv()            																										//Alexi Kessler
{
	frame* returnFrame = new frame();
	char* phlChar;
	phlChar = phl_recv();
	returnFrame = stringToFrame(phlChar);																							
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
		
		//cout<<"Checking getaddrinfo result"<<std::endl;
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
			cout<<"ConnectRes: "<<connectRes<<std::endl;
			cout<<"Error number: "<<strerror(errno)<<std::endl;
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
	char tempBuf[10];
	char lengthBuf[10];
	char sendChar[MAX_FRAME_SIZE+40];																								//Char* string to be sent with send()
	int length = 0;
	if (fr.seqNumber < 10)																											//Pad variables to ensure constant length of sent message
		sprintf(sendChar, "0000%hi%c%c", fr.seqNumber, fr.frameType, fr.EOP);														
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
	while (i<130)																													//Padding
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
	else if (fr.ED >= 10000)
		sprintf(tempBuf, "0%hi", fr.ED);
	else 
		DieWithError("Error converting ED to string");
	
	i = 0;
	while (i<6)																														//Add error detect value to sendChar
	{
		sendChar[length] = tempBuf[i];
		i++;
		length++;
	}
	
	sendChar[length] = fr.endPhoto;
	length++;
	
	//---------------------------------------FLIP A BIT-------------------------------------------------------
	if (numFramesSent%6 == 0)
	{
		int flipLoc = (rand()%6)+142;
		logFile<<"Introduced a bit error while sending"<<std::endl;
		if (DEBUG)
			cout<<"flipping bit in place: "<<flipLoc<<std::endl;
		sendChar[flipLoc] += 1;
	}
	//------------------------------------------------------------------------------------------------------------------
	
	i = 0;
	/*
	while (i<length)
	{
		if (sendChar[i] == 5)
			cout<<"sendChar["<<i<<"]: "<<"Ctrl+E"<<" ascii value:"<<(int)(sendChar[i])<<std::endl;
		else
			cout<<"sendChar["<<i<<"]: "<<sendChar[i]<<" ascii value:"<<(int)(sendChar[i])<<std::endl;
		i++;
	}
	cout<<"fr.endPhoto: "<<fr.endPhoto<<std::endl;
	*/
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
	{
		cout<<"Recv returned "<<bytesReceived<<std::endl;
		DieWithError("Issue with phl_recv");
	}
	/*
	int z = 0;
	if (DEBUG)
	{
		cout<<"Received "<<bytesReceived<<" bytes"<<std::endl;
		while (z < bytesReceived)
		{
			cout<<"Received["<<z<<"]: "<<recvBuf[z]<<" ascii value: "<<(int)recvBuf[z]<<std::endl;
			z++;
		}
	}*/
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
	char XOR[2];																													//Two bytes
	char tempChar1;
	char tempChar2;
	short int retVal;
	memset(XOR, 0, 2);
	while (counter < infoLength)
	{
		if (counter == 0)
		{
			strncpy(tempArray1, &frameInfo[0], 2);
			strncpy(tempArray2, &frameInfo[counter + 2], 2);
			tempChar1 = (char)(tempArray1[0]^tempArray2[0]);
			tempChar2 = (char)(tempArray1[1]^tempArray2[1]);
			strncpy(&XOR[counter],  &tempChar1,  1);
			strncpy(&XOR[counter+1], &tempChar2, 1);
			if (DEBUG)
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
	retVal = XOR[0] << 8 | XOR[1];																									//Turn XOR char array into short int
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
	cout<<"End Photo: "<<fr.endPhoto<<std::endl;
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
	if (DEBUG)
		cout<<"WRITING PACKET"<<numPacketsSent<<" TO FILE"<<std::endl;
	FILE *out = fopen("testWrite.jpg", "a");
	//fprintf(out, "%s %d\n", "\nWriting packet", numPacketsSent);
	int count = 0;
	while (count < packet.dataLength)
	{
		fprintf(out, "%c", packet.payload[count]);
		count++;
	}
	//fprintf(out, "%s", "\nFinished writing packet\n");
	fclose(out);

}

frame* stringToFrame(char* buffer){																									//Slightly adapted from code by Juan Rodriguez
	//cout<<"Recevied string: "<<buffer<<std::endl;
	
	frame* new_frame = new frame();
	
	int i = 0;
	int place = 0;
	int startFrameType = SEQ_NUM_SIZE;
	int startEOP = startFrameType + FRAME_TYPE_SIZE;
	int startUsableBytes = startEOP + EOP_SIZE;
	int startPayload = startUsableBytes + USABLE_BYTES;
	int startERRD = startPayload + PAYLOAD_SIZE;
	char seq_num[SEQ_NUM_SIZE + 1];
	char usable_b[USABLE_BYTES + 1];
	char ED[ERRD_SIZE + 1];
		
	memset(seq_num, 0, strlen(seq_num));
	
	if (DEBUG)
		cout<<"Parsing sequence number...\n";
	while (i < place+SEQ_NUM_SIZE){
		
		strncat(&seq_num[i], &buffer[i], 1);
		i++;	
	}
	seq_num[SEQ_NUM_SIZE] = '\0';
	
	place=i;
	
	if (DEBUG)
		cout<<"Parsing frame type...\n";
	while (i< place+FRAME_TYPE_SIZE)
	{
		strncat(&new_frame->frameType, &buffer[i], 1);
		i++;
	}
	
	place=i;
	
	if (DEBUG)
		cout<<"Parsing EOP...\n";
	while (i < place+EOP_SIZE)
	{	
		strncpy(&new_frame->EOP, &buffer[i], 1);
		i++;
	}
	place =i;
	
	if (DEBUG)
		cout<<"Parsing usable bytes...\n";
	while (i < place+USABLE_BYTES)
	{
		strncpy(&usable_b[i-startUsableBytes], &buffer[i], 1);
		i++;
	}
	usable_b[USABLE_BYTES] = '\0';
	
	place=i;
	
	if (DEBUG)
		cout<<"Parsing payload...\n";
	int bytes_rcvd = atoi(usable_b);
	//cout<<bytes_rcvd<<std::endl;

	while (i < place+PAYLOAD_SIZE)
	{
		strncpy(&new_frame->payload[i-startPayload], &buffer[i], 1);
		i++;
	}
	//memset(&new_frame->payload[bytes_rcvd], 0, PAYLOAD_SIZE - bytes_rcvd);
	
	place=i;	
	
	if (DEBUG)
		cout<<"Parsing Error Detection...\n";
	while(i < place+ERRD_SIZE)
	{
		strncpy(&ED[i-startERRD], &buffer[i], 1);
		i++;
	}

	new_frame->endPhoto = buffer[i];
	
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
		else
		{
			break;
		}
		parse++;
	}
	
	if(DEBUG)
	{
		cout<<"Parsed Error Detection Value is: "<<ED<<std::endl;
	}
	new_frame->seqNumber = atoi(seq_num);
	/***********************************NEEDS TO BE CHECKED****************************/
	//new_frame->payload[PAYLOAD_SIZE] = '\0';
	
	new_frame->dataLength = bytes_rcvd;
	new_frame->ED = atoi(&ED[ignore]);
	
	if (DEBUG)
		printFrame(*new_frame);
	return new_frame;	
}

