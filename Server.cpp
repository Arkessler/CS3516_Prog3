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

#define DATA_FRAME 'D'
#define ACK_FRAME 'A'
#define END_PACKET 'E'
#define CONT_PACKET 'C'
#define END_PHOTO 'Y'
#define CONT_PHOTO 'N'


#define MAXPENDING 5 /* Maximum outstanding connection requests */
#define BUFFSIZE 150   /* Size of receive buffer */
#define SERVPORT 1199	
#define MAX_PACKET_PAYLOAD 256
#define FRAME_TYPE_SIZE 1
#define SEQ_NUM_SIZE 5
#define EOP_SIZE 1
#define PAYLOAD_SIZE 130
#define ERRD_SIZE 6
#define MAX_FRAME_PAYLOAD 130
#define USABLE_BYTES 5
#define SIZE_CLIENTID 4
#define SIZE_PNUM 2
#define SIZE_ENDPHOTO 1

#define DEBUG 0
#define DEBUG_0 0
#define DEBUG_1 1
#define DEBUG_2 0

class packet
{
	public:
		char payload[256]; 		
		unsigned char datalenght;																									//256 bytes long
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
		short int ED;			
		char endPhoto;																									//2 bytes long
};

void DieWithError(char *errorMessage); /* Error handling function */
frame* read_frame(char* buffer);
void build_packet();
void printFrame (frame fr);
void get_init_pck(int client_id,int p_num,char* buffer);
packet* build_packet(frame* frame_array[]);
void printPacket(packet pck);
void build_frame_ack(frame* frame_recieved,char* ack_frame);


int main(int argc, char *argv[]){ 
 int bytes_r;
 int pid;
 int frames_count;								//Loop counter
 char buffer[BUFFSIZE];
 int servSock; 					/*Socket descriptor for server */
 int clntSock; 					/* Socket descriptor for client */
 struct sockaddr_in ServAddr;		/* Local address */
 struct sockaddr_in ClntAddr; 		/* Client address */
 unsigned short ServPort; 			/* Server port */
 unsigned int clntLen;
 frame* frameArray[10];
 char* test_string = "Hello Master";
 ofstream picture;
 int client_id, p_num;
 int frame_size = SEQ_NUM_SIZE+FRAME_TYPE_SIZE+EOP_SIZE+USABLE_BYTES+MAX_FRAME_PAYLOAD+ERRD_SIZE+SIZE_ENDPHOTO;

 packet* new_packet = new packet();
 

ServPort = SERVPORT;			
/* Variables declared*/

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
        		
                close(servSock);

                if((bytes_r = recv(clntSock, buffer, BUFFSIZE - 1, 0)) < 0)
                	        cout<< "Error recieving initial packet\n";

               	cout<<"String recieved: :"<<buffer<<std::endl;

               	get_init_pck(client_id, p_num, buffer);
               	p_num = 1;
               	//While x < number of photos
               	int photo_num = 0;
            do{
               	
            	photo_num++;
               	//do while x < not end of photo
					
               		/*Build file name*/
               		std::string photo_name;

               		std::stringstream tempStr;

               		tempStr.str(std::string());	

               		photo_name = "photonew";
	               	tempStr<<client_id;																											//Convert cId to string
					photo_name.append(tempStr.str());
					tempStr.str(std::string());																								//Reset tempStr
					tempStr<<photo_num;																											//Convert count to string
					photo_name.append(tempStr.str());   																						//THIS MAY NEED TO BE CHANGED TO 1 INSTEAD OF COUNT
					tempStr.str(std::string());																								//Reset tempStr
					photo_name.append(".txt");

					cout<<"Photo name: "<<photo_name<<std::endl;

					ofstream stream (photo_name.c_str(), std::ios::binary|std::ios::app);
					int packet_count = 0;
               	do{               
               		packet_count++;
               		frames_count = 0;

               		frame* inc_frame = new frame();
               		//Recieve frames to form packet

               		cout<<"\n\nRecieving packet "<<packet_count<<"...\n\n\n";

	                do{

	                	memset(buffer, 0, sizeof(buffer));

	                	if((bytes_r = recv(clntSock, buffer, BUFFSIZE - 1, 0)) < 0)
	                	        cout<< "Error in recv()";
					
	                	//buffer[bytes_r] = '\0';
						cout<<"Bytes recieved: "<<bytes_r<<std::endl;
	                	
	                	

	                	if(DEBUG){               
	                		int x = 0;

	                		while(x < BUFFSIZE){
	                		cout<<"Buffer["<<x<<"]: "<<buffer[x]<<" ascii val: "<<(int) buffer[x]<<std::endl;
	                		x++;
	                		}
	                	}
						
						cout<<"Buffer: "<<buffer<<std::endl;
	                	//Send AK
						inc_frame = read_frame(buffer);

						int frame_size = SEQ_NUM_SIZE+FRAME_TYPE_SIZE+EOP_SIZE+USABLE_BYTES+MAX_FRAME_PAYLOAD+ERRD_SIZE+SIZE_ENDPHOTO;
						char tempString[frame_size];
						/*
						while (temp<(frames_count+2))																									//Padding based on i size
						{
							tempString[temp] = 'P';
							temp++;
						}
						while (temp<strlen(test_string)+(frames_count+2))
						{
							tempString[temp] = test_string[temp-(frames_count+2)];
							temp++;
						}
						*/
						//if(inc_frame != NULL){

							build_frame_ack(inc_frame, tempString);

							cout<<"Frame ack to be sent: "<<tempString<<std::endl;

							//cout<<"Acknowledgement string to be sent: "<<(tempString)<<std::endl;
							int sendRes = send(clntSock, tempString, strlen(tempString), 0);
							if (sendRes < 0)
								DieWithError("Send failed");

							else 
							{
								if (DEBUG)
									cout<<"Bytes sent: "<<sendRes<<std::endl;
							}
						//}

						frameArray[frames_count] = inc_frame;					//Store the frames to later build up 
						frames_count++;

						//cout<<"Frame:\n"<<inc_frame->payload<<std::endl;
						cout<<"Frames_count value after increment: "<<frames_count<<std::endl;



	        		} while(inc_frame->EOP != 'E');

	        		packet* new_packet = new packet();

	        		cout<<"\n\nBuilding packet...\n\n\n";
	        		new_packet = build_packet(frameArray);

	        		cout<<"Printing packet...\n\n\n";
	        		printPacket(*new_packet);

	        		stream.write("I am so cool", 18);

	        		cout<<"\n\nWritting to file...\n\n\n";
	        		stream.write((*new_packet).payload, MAX_FRAME_PAYLOAD);

	        		cout<<"\n\nEnd of photo: "<<new_packet->endPhoto<<std::endl;


	        	} while(new_packet->endPhoto != 'E');

	        	stream.close();
	        	cout<<"\n\nClosed socket...\n\n\n";

	        } while(photo_num < p_num);
	        	
        	close(clntSock);
            exit(0);
    	}

        else if (pid > 0){
        close(clntSock);  // parent doesn't need this
    }

 }
}
 /* NOT REACHED */


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
	int startENDP = startERRD + SIZE_ENDPHOTO;
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
		
		if(DEBUG_0){
		cout<<"Value of i: "<<i<<"\nValue of place: "<<place<<std::endl;
		cout<<"Starting copy..."<<std::endl;
		}
		strncat(&seq_num[i], &buffer[i], 1);
		
		if(DEBUG_0){
		cout<<"\nValue of SEQ_NUM: "<<seq_num<<std::endl;
		}
		
		i++;	
	}
	seq_num[SEQ_NUM_SIZE] = '\0';
	cout<<"Parsed sequence number: "<<seq_num<<std::endl;
	
	if(DEBUG_0){
	cout<<"Value i: "<<i<<std::endl;
	}
	
	place=i;
	
	cout<<"Parsing frame type...\n";
	while (i< place+FRAME_TYPE_SIZE){
		strncat(&new_frame->frameType, &buffer[i], 1);
		i++;
	}
	cout<<"Parsed frame type: "<<frameType<<std::endl;
	
	if(DEBUG_0){
	cout<<"Value i: "<<i<<"\nPlace: "<<place<<std::endl;
	}
	
	place=i;
	
	cout<<"Parsing EOP...\n";
	while (i < place+EOP_SIZE){
		
		if(DEBUG_0){
		cout<<"EOP while 'i' value: "<<i<<std::endl;
		}
		
		strncpy(&new_frame->EOP, &buffer[i], 1);
		i++;
	}
	cout<<"Parsed EOP...\n";
	 place =i;
	
	if(DEBUG_0){
	cout<<"Value i: "<<i<<"\nPlace: "<<place<<std::endl;
	}
	
	cout<<"Parsing usable bytes...\n";
	while (i < place+USABLE_BYTES){
		
		if(DEBUG_0){
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

	if(DEBUG_0)
		cout<<bytes_rcvd<<std::endl;
	

	while (i < place+PAYLOAD_SIZE){
		
		if(DEBUG_0)
			cout<<"Value i: "<<i<<std::endl;
		
		
		strncpy(&new_frame->payload[i-startPayload], &buffer[i], 1);
		i++;
		
		if(DEBUG_0){
		cout<<"Value i: "<<i<<std::endl;
		cout<<"Payload["<<i-startPayload<<"] "<<&payload[i-startPayload]<<std::endl;
		}
	}
	//memset(&new_frame->payload[bytes_rcvd], 0, PAYLOAD_SIZE - bytes_rcvd);
	cout<<"Parsed payload...\n";
	
	place=i;
	
	if(DEBUG_0){
	cout<<"Value i: "<<i<<"\nPlace: "<<place<<std::endl;
	}
	
	cout<<"Parsing Error Detection...\n";
	while(i < place+ERRD_SIZE){
		strncpy(&ED[i-startERRD], &buffer[i], 1);
		i++;
	}
	place = i;
	if(DEBUG_0){
	cout<<"Error Detection string: "<<ED<<std::endl;
	}
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

	while(i < place+SIZE_ENDPHOTO){
		new_frame->endPhoto = buffer[i];
		i++;
	}

	if(DEBUG_0){

	cout<<"Ignore: "<<ignore<<std::endl;

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

packet* build_packet(frame* frame_array[]){

	packet* new_packet = new packet();
	int frames_processed, bytes_r, total_bytes_r;

	frames_processed = 0;
	//bytes_r = 0;
	total_bytes_r = 0;

	//Posibly need to rearrange array

	//while(frames_processed < frames_count){
	while(total_bytes_r < MAX_PACKET_PAYLOAD){
		
		frame* tempFrame = new frame();
		
		bytes_r = 0;
		
		tempFrame = frame_array[frames_processed];

		cout<<"Processing frame "<<tempFrame->seqNumber<<std::endl;

		printFrame(*tempFrame);
		//if(total_bytes_r < MAX_PACKET_PAYLOAD){
			while(bytes_r < tempFrame->usable_bytes){
				
				if(DEBUG_2)
					cout<<"\n\nFrame payload["<<bytes_r<<"] "<<tempFrame->payload[bytes_r]<<std::endl;
				
				new_packet->payload[total_bytes_r] = tempFrame->payload[bytes_r];
				
				
				if(DEBUG_2){
					cout<<"Payload["<<total_bytes_r<<"] "<<(int) new_packet->payload[total_bytes_r]<<std::endl;
				}

				bytes_r++;
				total_bytes_r++;
			}
		frames_processed++;
		//total_bytes_r += bytes_r;
		//}

		if(tempFrame->endPhoto == END_PHOTO){;
			new_packet->endPhoto = END_PHOTO;
		}

		if(tempFrame->EOP == 'E'){

			cout<<"Reached end of packet\n";
			break;
		}
	}

	return new_packet;
}

void get_init_pck(int client_id,int p_num,char* buffer){
	char tempClID[SIZE_CLIENTID];
	char tempPNUM[SIZE_PNUM];
	int i, place;
	int startPNUM = SIZE_CLIENTID;

	i = 0;
	place = 0;
	
	while(i < place+SIZE_CLIENTID){
		strncpy(&tempClID[i], &buffer[i], 1);
		i++;
	}
	tempClID[SIZE_CLIENTID] = '\0';

	place = i;

	while(i < place + SIZE_PNUM){
		strncpy(&tempPNUM[i-startPNUM], &buffer[i], 1);
		i++;
	}

	if(DEBUG_1){
		cout<<"Temp num string: "<<tempPNUM<<std::endl;
	}

	tempPNUM[SIZE_PNUM] = '\0';

	client_id = atoi(tempClID);
	p_num = atoi(tempPNUM);

	cout<<"Client ID: "<<client_id<<"\n";
	cout<<"Photo number: "<<p_num<<"\n";
}

void printPacket(packet pck){


	//cout<<"Packet Payload:"<<std::endl; 

	int i = 0;
	
	while (i < MAX_PACKET_PAYLOAD) 
	{
		cout<<"packet Payload["<<i<<"]: "<<pck.payload[i]<<" acii value:"<<(int)pck.payload[i]<<std::endl;
		i++;
	}
	
	//cout<<"Error Detection: "<<fr.ED<<std::endl;
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
	cout<<"End photo:"<<fr.endPhoto<<std::endl;
}

void build_frame_ack(frame* frame_recieved, char* ack_frame){

	char tempERRD[ERRD_SIZE+2];
	char endPhoto = '0';
	char usable_bytes_buff[USABLE_BYTES+2];
	//char ack_frame[SEQ_NUM_SIZE+FRAME_TYPE_SIZE+EOP_SIZE+USABLE_BYTES+MAX_FRAME_PAYLOAD+ERRD_SIZE+SIZE_ENDPHOTO];
	char fake_payload[MAX_FRAME_PAYLOAD+ERRD_SIZE+1];
	int length = 0;

	sprintf(ack_frame, "%04hi%c0", frame_recieved->seqNumber, ACK_FRAME);
	length += (SEQ_NUM_SIZE+FRAME_TYPE_SIZE+EOP_SIZE);
	
	sprintf(usable_bytes_buff, "%04hi", 0);
	
	strcat(ack_frame, usable_bytes_buff);
	length += USABLE_BYTES;
	
	int i = 0;
	while (i<(MAX_FRAME_PAYLOAD))
	{
		ack_frame[length] = '0';
		length++; 
		i++;
	}

	sprintf(tempERRD, "%05hi", frame_recieved->seqNumber);

	length += ERRD_SIZE;

	strcat(ack_frame, tempERRD);
	ack_frame[length] = endPhoto;


/*
	while (i<130)											//Padding
	{
		sendChar[length] = '0';
		length++;
		i++;
	}
*/

/*
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
		DieWithError("Error converting ED to string");																											//2 bytes long
*/

	//return ack_frame;
}