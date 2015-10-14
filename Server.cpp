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
#define MAX_FRAME_SIZE 149

#define DEBUG 0
#define DEBUG_0 0
#define DEBUG_1 0
#define DEBUG_2 0
#define DEBUG_6 0
#define DEBUG_SPECIAL 1
#define ACK_CORRUPTION 0
class packet
{
	public:
		char payload[256]; 																										//256 bytes long
		char endPhoto;		
		unsigned short int datalenght;																											//1 byte long end of photo indicator
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
void get_init_pck(int& client_id,int& p_num,char* buffer);
packet* build_packet(frame* frame_array[]);
void printPacket(packet pck);
void build_frame_ack(frame* frame_recieved,char* ack_frame);
void build_packet_ack(packet* packet_assembledpac,char* ack_packet);
short int errorDetectCreate(char* info, int infoLength);
void check_error(char* buffer, char* error_detect_char);
void initial_print();

/*Global Variables*/

std::ofstream logFile;
std::string logLoc;
int packet_count;
int total_frames_recieved;
 

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
 ofstream stream;
 int client_id, p_num;
 int frame_size = SEQ_NUM_SIZE+FRAME_TYPE_SIZE+EOP_SIZE+USABLE_BYTES+MAX_FRAME_PAYLOAD+ERRD_SIZE+SIZE_ENDPHOTO;
 int errorDetectString_size = SEQ_NUM_SIZE+FRAME_TYPE_SIZE+EOP_SIZE+USABLE_BYTES+MAX_FRAME_PAYLOAD;
 char errorDetectString[errorDetectString_size];
 packet* new_packet = new packet();
 int frame_is_bad;
 //int frame_size = SEQ_NUM_SIZE+FRAME_TYPE_SIZE+EOP_SIZE+USABLE_BYTES+MAX_FRAME_PAYLOAD+ERRD_SIZE+SIZE_ENDPHOTO;
 char tempString[frame_size];


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

 	initial_print();
 	

	if((pid = fork()) < 0){
                printf("Error in fork()");
        }

        if (pid == 0) { // this is the child process
        		
                close(servSock);

                if((bytes_r = recv(clntSock, buffer, BUFFSIZE - 1, 0)) < 0)
                	        cout<< "Error recieving initial packet\n";

               	cout<<"String recieved: :"<<buffer<<std::endl;

               	get_init_pck(client_id, p_num, buffer);

               	cout<<"Client ID returned: "<<client_id<<"\nNumber of photos: "<<p_num<<std::endl;
               	
               	std::stringstream clientString;
               	std::stringstream fileType;

               	clientString.str(std::string());
               	clientString<<client_id;

               	fileType.str(std::string());
               	fileType<<".log";

               	logLoc = "Server_";
				//clientString<<".log";
				logLoc.append(clientString.str());
				logLoc.append(fileType.str());
				fileType.str(std::string());

				cout<<"Log location: "<<logLoc.c_str()<<std::endl;

				logFile.open(logLoc.c_str());
				logFile<<"\nConnected to client "<<client_id<<std::endl;

              	//p_num = 1;
               	//While x < number of photos
               	int photo_num = 0;
            do{
               	
            	photo_num++;
               	//do while x < not end of photo
					
               		/*Build file name*/
               		std::string photo_name;

               		
               		std::stringstream photoString;

               			
               		photoString.str(std::string());

               		photo_name = "photonew";
               		//cout<<"Client id to store: "<<client_id<<std::endl;
	               	
	               																												//Convert cId to string
					photo_name.append(clientString.str());
					//tempStr.str(std::string());
					photoString.width(SIZE_PNUM);																								//Reset tempStr
					photoString.fill('0');
					photoString<<photo_num;																											//Convert count to string
					photo_name.append(photoString.str());   																						//THIS MAY NEED TO BE CHANGED TO 1 INSTEAD OF COUNT
					//tempStr.str(std::string());
					fileType<<".txt";

					photo_name.append(fileType.str());
					
					
					logFile<<"Attempting to recieve photo number "<<photo_num<<" on file "<<photo_name<<std::endl;
					
					FILE * zeroOut = fopen(photo_name.c_str(),"w");
					fprintf(zeroOut, "");
					fclose(zeroOut);
					
					cout<<"Photo name: "<<photo_name<<std::endl;

					

					packet_count = 0;
					//int w=0;
					total_frames_recieved = 0;
               	do{               
               		packet_count++;
               		frames_count = 0;

               		frame* inc_frame = new frame();
               		//Recieve frames to form packet

               		cout<<"\n\nRecieving packet "<<packet_count<<"...\n\n";


	                do{
	                	//frame_is_bad = 0;

	                	memset(buffer, 0, sizeof(buffer));
	                	memset(errorDetectString, 0, sizeof(errorDetectString));

	                	if((bytes_r = recv(clntSock, buffer, BUFFSIZE - 1, 0)) < frame_size){
	                	    DieWithError("Error in recv()");
	                	}
					
						cout<<"Bytes recieved: "<<bytes_r<<std::endl;

	                	if(DEBUG){               
	                		int x = 0;

	                		while(x < BUFFSIZE){
	                		cout<<"Buffer["<<x<<"]: "<<buffer[x]<<" ascii val: "<<(int) buffer[x]<<std::endl;
	                		x++;
	                		}
	                	}

						inc_frame = read_frame(buffer);
						total_frames_recieved++;

						check_error(buffer, errorDetectString);

						short int frame_read_error = errorDetectCreate(errorDetectString, errorDetectString_size);
						
						cout<<"From frame: "<<inc_frame->seqNumber<<"\nError generated: "<<frame_read_error<<"\nED from frame: "<<inc_frame->ED<<std::endl;
						
						if(frame_read_error == inc_frame->ED){
							frame_is_bad = 0;
							//cout<<"\n\n\nAt least error detection is working now\n\n\n";
							
							logFile<<"\nFrame "<< inc_frame->seqNumber<<" recieved correctly."<<std::endl;

						//int frame_size = SEQ_NUM_SIZE+FRAME_TYPE_SIZE+EOP_SIZE+USABLE_BYTES+MAX_FRAME_PAYLOAD+ERRD_SIZE+SIZE_ENDPHOTO;
						//char tempString[frame_size];
				

							build_frame_ack(inc_frame, tempString);

							//cout<<"Frame ack to be sent: "<<tempString<<std::endl;

							int sendRes = send(clntSock, tempString, strlen(tempString), 0);
							if (sendRes < 0){
								DieWithError("Send failed");
							}

							

							else 
							{
								if (DEBUG)
									cout<<"Bytes sent: "<<sendRes<<std::endl;
							}

							logFile<<"Acknolegment for frame "<< inc_frame->seqNumber<<" sent."<<std::endl;
							cout<<"\nBytes sent in ack frame: "<<sendRes<<std::endl;

							

						frameArray[frames_count] = inc_frame;					//Store the frames to later build up 
						frames_count++;
					}

					else{
						cout<<"\nRecieved a bad frame...\nTotal frames recieved: "<<total_frames_recieved<<"\n";
						frame_is_bad = 1;
						logFile<<"\nFrame "<< inc_frame->seqNumber<<" recieved with error.\nWaiting for retransmission\n";

						if(DEBUG_SPECIAL){
							int x = 0;
							if(packet_count > 5000){
	                		cout<<"\nBuffer recieved: \n";
	                		
	                		cout<<"Error detection called with lenght: "<<errorDetectString_size<<std::endl;

	                		while(x < errorDetectString_size){
	                			cout<<"Buffer["<<x<<"]: "<<errorDetectString[x]<<" ascii val: "<<(int) errorDetectString[x]<<std::endl;
	                			x++;
	                		}

	                		cout<<"Return value from error detect: "<<frame_read_error<<std::endl;
	                		}
	                	}
					}
						//cout<<"Frame:\n"<<inc_frame->payload<<std::endl;
						if(DEBUG){
							cout<<"\nFrames_count value after increment: "<<frames_count<<std::endl;
						}


						//w++;
	        		} while(inc_frame->EOP != END_PACKET || frame_is_bad);

	        		packet* new_packet = new packet();

	        		cout<<"\nBuilding packet...\n";
	        		new_packet = build_packet(frameArray);
	        		logFile<<"\nBuilt packet "<<packet_count<<std::endl;

	        		memset(tempString, 0, sizeof(tempString));

	        		build_packet_ack(new_packet, tempString);
	        		
	        		int sendRes = send(clntSock, tempString, strlen(tempString), 0);
						if (sendRes < 0){
							DieWithError("Send to network layer failed");
						}

					logFile<<"Sent newlork layer ack for packet\n";
					cout<<"\nWriting to file...\n";
	        		
					FILE * out = fopen(photo_name.c_str(), "a");
					int count = 0;
					while (count < new_packet->datalenght)
					{
						fprintf(out, "%c", new_packet->payload[count]);
						count++;
					}
					
					cout<<"Bytes written: "<<count<<std::endl;
					
					fclose(out);  

	        		cout<<"\nEnd of photo: "<<new_packet->endPhoto<<std::endl;


	        	}while((new_packet->endPhoto) != 'Y');

	        	cout<<"\nSuccesfully recieved photo number "<<photo_num<<std::endl;
	        	logFile<<"\nSuccesfully recieved photo number "<<photo_num<<std::endl;

	        	//stream.close();

	        } while(photo_num < p_num);
	        
	        logFile<<"\nRecieved "<<p_num<<" photos, closing connection to client "<<client_id<<std::endl;

	        cout<<"\n\nClosed socket...\n\n";
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

	cout<<"\n\nIn read_frame(): \n";

	frame* new_frame = new frame();
	
	int i = 0;
	int place = 0;
	int startFrameType = SEQ_NUM_SIZE;
	int startEOP = startFrameType + FRAME_TYPE_SIZE;
	int startUsableBytes = startEOP + EOP_SIZE;
	int startPayload = startUsableBytes + USABLE_BYTES;
	int startERRD = startPayload + PAYLOAD_SIZE;
	//int startENDP = startERRD + SIZE_ENDPHOTO;
	char seq_num[SEQ_NUM_SIZE + 1];
	//int ED_temp;
	char frameType;
	char usable_b[USABLE_BYTES + 1];
	//char EOP;
	char ED[ERRD_SIZE + 1];
	char payload[130];
		
	memset(seq_num, 0, strlen(seq_num));
	
	//cout<<"Parsing sequence number...\n";
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
	//cout<<"Parsed sequence number: "<<seq_num<<std::endl;
	
	if(DEBUG_0){
	cout<<"Value i: "<<i<<std::endl;
	}
	
	place=i;
	
	//cout<<"Parsing frame type...\n";
	while (i< place+FRAME_TYPE_SIZE){
		strncat(&new_frame->frameType, &buffer[i], 1);
		i++;
	}
	//cout<<"Parsed frame type: "<<frameType<<std::endl;
	
	if(DEBUG_0){
	cout<<"Value i: "<<i<<"\nPlace: "<<place<<std::endl;
	}
	
	place=i;
	
	//cout<<"Parsing EOP...\n";
	while (i < place+EOP_SIZE){
		
		if(DEBUG_0){
		cout<<"EOP while 'i' value: "<<i<<std::endl;
		}
		
		strncpy(&new_frame->EOP, &buffer[i], 1);
		i++;
	}
	//cout<<"Parsed EOP...\n";
	 place =i;
	
	if(DEBUG_0){
	cout<<"Value i: "<<i<<"\nPlace: "<<place<<std::endl;
	}
	
	//cout<<"Parsing usable bytes...\n";
	while (i < place+USABLE_BYTES){
		
		if(DEBUG_0){
		cout<<"EOP while 'i' value: "<<i<<std::endl;
		}
		
		strncpy(&usable_b[i-startUsableBytes], &buffer[i], 1);
		i++;
	}
	usable_b[USABLE_BYTES] = '\0';
	//cout<<"Printing usable_b string: "<<usable_b<<std::endl;

	//cout<<"Parsed usable bytes...\n";
	
	place=i;
	
	//cout<<"Parsing payload...\n";

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
	//cout<<"Parsed payload...\n";
	
	place=i;
	
	if(DEBUG_0){
	cout<<"Value i: "<<i<<"\nPlace: "<<place<<std::endl;
	}
	
	//cout<<"Parsing Error Detection...\n";
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

		else{
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
	
	//printFrame(*new_frame);
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

		cout<<"\nProcessing frame "<<tempFrame->seqNumber<<std::endl;

		//printFrame(*tempFrame);
		
			while(bytes_r <= tempFrame->usable_bytes){
				
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
	
		if(tempFrame->endPhoto == END_PHOTO){
			new_packet->endPhoto = END_PHOTO;
			cout<<"Reached end of photo...\n";
			break;
		}
		else if(tempFrame->endPhoto == 'N'){
			new_packet->endPhoto = 'N';
		}
		
		//cout<<"Packet endPhoto after Processing"
		if(tempFrame->EOP == 'E'){

			cout<<"Reached end of packet\n";
			break;
		}
		
	}
		
	total_bytes_r--;
	new_packet->datalenght = total_bytes_r;

	return new_packet;
	
}

void get_init_pck(int& client_id,int& p_num,char* buffer){
	char tempClID[SIZE_CLIENTID];
	char tempPNUM[SIZE_PNUM + 1];

	int i, place;
	int startPNUM = SIZE_CLIENTID;

	i = 0;
	place = 0;

	cout<<"String recieved: "<<buffer<<std::endl;
	
	while(i < place+SIZE_CLIENTID){
		tempClID[i] = buffer[i];
		i++;
		if(DEBUG_1){
		cout<<"value of i in loop: "<<i<<std::endl;
		}
	}
	if(DEBUG_1){
		cout<<"value of i: "<<i<<std::endl;
	}

	//tempClID[SIZE_CLIENTID] = '\0';

	if(DEBUG_1){
		cout<<"value of i: "<<i<<std::endl;
	}

	client_id = atoi(tempClID);

	place = i;

	if(DEBUG_1){
		cout<<"value of i: "<<i<<std::endl;
	}

	while(i < place + SIZE_PNUM){
		strncpy(&tempPNUM[i-startPNUM], &buffer[i], 1);
		i++;
	}

	tempPNUM[SIZE_PNUM] = '\0';
	if(DEBUG_1){
		cout<<"Temp num string: "<<tempPNUM<<std::endl;
	}

	

	//client_id = atoi(tempClID);
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
	cout<<"\n\nFrame read: \n";
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

void build_frame_ack(frame* frame_recieved, char* ack_frame)
{

	char tempERRD[ERRD_SIZE+2];
	//char endPhoto = '0';
	char usable_bytes_buff[USABLE_BYTES+2];
	//char ack_frame[SEQ_NUM_SIZE+FRAME_TYPE_SIZE+EOP_SIZE+USABLE_BYTES+MAX_FRAME_PAYLOAD+ERRD_SIZE+SIZE_ENDPHOTO];
	//char fake_payload[MAX_FRAME_PAYLOAD+ERRD_SIZE+1];
	int length = 0;

	sprintf(ack_frame, "%05hi%c0", frame_recieved->seqNumber, ACK_FRAME);
	length += (SEQ_NUM_SIZE+FRAME_TYPE_SIZE+EOP_SIZE);
	
	if(DEBUG_6){
		cout<<"\n\nAck frame before usable_bytes: "<<ack_frame<<std::endl;
	}

	sprintf(usable_bytes_buff, "%05hi", 0);
	
	strcat(ack_frame, usable_bytes_buff);
	length += USABLE_BYTES;
	
	if(DEBUG_6){
		cout<<"\n\nAck frame before payload: "<<ack_frame<<std::endl;
	}
	
	int i = 0;
	char temp= '0';
	while (i<MAX_FRAME_PAYLOAD)
	{
		strncat(ack_frame, &temp, 1);
		length++; 
		i++;
	}


	sprintf(tempERRD, "%06hi", frame_recieved->seqNumber);

	if(DEBUG_6){
		cout<<"\n\nTemp Error Detection: "<<tempERRD<<std::endl;
	}

	length += ERRD_SIZE;

	if(DEBUG_6){
		cout<<"\n\nAck frame before Error Detection: "<<ack_frame<<std::endl;
	}

	strcat(ack_frame, tempERRD);
	strncat(ack_frame, &temp, 1);
	
	if(DEBUG_6){
		cout<<"\n\nAck frame to be sent: "<<ack_frame<<std::endl;
	}

	if(ACK_CORRUPTION){
		if((total_frames_recieved % 11) == 0){
			int flipLoc = (rand()%6)+142;
			logFile<<"Introduced a bit error while sending"<<std::endl;
			if (DEBUG_6)
				cout<<"flipping bit in place: "<<flipLoc<<std::endl;
			ack_frame[flipLoc] += 1;
		}
	}

	//return ack_frame;
}

void build_packet_ack(packet* packet_assembled, char* ack_packet)
{

	char tempERRD[ERRD_SIZE+2];
	//char endPhoto = '0';
	char usable_bytes_buff[USABLE_BYTES+2];
	//char ack_packet[SEQ_NUM_SIZE+FRAME_TYPE_SIZE+EOP_SIZE+USABLE_BYTES+MAX_FRAME_PAYLOAD+ERRD_SIZE+SIZE_ENDPHOTO];
	//char fake_payload[MAX_FRAME_PAYLOAD+ERRD_SIZE+1];
	int length = 0;

	sprintf(ack_packet, "%05hi%c0", packet_count, DATA_FRAME);
	length += (SEQ_NUM_SIZE+FRAME_TYPE_SIZE+EOP_SIZE);
	
	if(DEBUG_6){
		cout<<"\n\nAck frame before usable_bytes: "<<ack_packet<<std::endl;
	}

	sprintf(usable_bytes_buff, "%05hi", 0);
	
	strcat(ack_packet, usable_bytes_buff);
	length += USABLE_BYTES;
	
	if(DEBUG_6){
		cout<<"\n\nAck frame before payload: "<<ack_packet<<std::endl;
	}
	
	int i = 0;
	char temp= '0';
	while (i<MAX_FRAME_PAYLOAD)
	{
		if(i == 1){
			char m = ACK_FRAME;
			strncat(ack_packet, &m, 1);
			i++;
			length++;
		}

		else{
		strncat(ack_packet, &temp, 1);
		length++; 
		i++;
		}
	}


	sprintf(tempERRD, "%06hi", packet_count);

	if(DEBUG_6){
		cout<<"\n\nTemp Error Detection: "<<tempERRD<<std::endl;
	}

	length += ERRD_SIZE;

	if(DEBUG_6){
		cout<<"\n\nAck frame before Error Detection: "<<ack_packet<<std::endl;
	}

	strcat(ack_packet, tempERRD);
	strncat(ack_packet, &temp, 1);
	
	if(DEBUG_6){
		cout<<"\n\nAck frame to be sent: "<<ack_packet<<std::endl;
	}


	//return ack_frame;
}

void check_error(char* buffer, char* error_detect_char)
{
	cout<<"\n\nIn check_error(): \n";
	int i = 0;
	int place = 0;
	int startFrameType = SEQ_NUM_SIZE;
	int startEOP = startFrameType + FRAME_TYPE_SIZE;
	int startUsableBytes = startEOP + EOP_SIZE;
	int startPayload = startUsableBytes + USABLE_BYTES;
	//int startERRD = startPayload + PAYLOAD_SIZE;
	//char error_detect_char[startERRD];

	char seq_num[SEQ_NUM_SIZE + 1];
	char payload[130];
		
	memset(seq_num, 0, strlen(seq_num));
	
	if(DEBUG_0){
		cout<<"Parsing sequence number...\n";
	}

	while (i < place+SEQ_NUM_SIZE){
		
		if(DEBUG_0){
		cout<<"Value of i: "<<i<<"\nValue of place: "<<place<<std::endl;
		cout<<"Starting copy..."<<std::endl;
		}
		strncpy(&error_detect_char[i], &buffer[i], 1);
		
		if(DEBUG_0){
		cout<<"\nValue of SEQ_NUM: "<<seq_num<<std::endl;
		}
		
		i++;	
	}
	//seq_num[SEQ_NUM_SIZE] = '\0';
	if(DEBUG_0){
		cout<<"Parsed sequence number: "<<error_detect_char<<std::endl;
	}
	
	if(DEBUG_0){
	cout<<"Value i: "<<i<<std::endl;
	}
	
	place=i;
	
	if(DEBUG_0){
		cout<<"Parsing frame type...\n";
	}

	while (i< place+FRAME_TYPE_SIZE){
		strncpy(&error_detect_char[i], &buffer[i], 1);
		i++;
	}
	if(DEBUG_0){
		cout<<"Parsed frame type: "<<error_detect_char<<std::endl;
	
		cout<<"Value i: "<<i<<"\nPlace: "<<place<<std::endl;
	}
	
	place=i;
	
	if(DEBUG_0){
		cout<<"Parsing EOP...\n";
	}

	while (i < place+EOP_SIZE){
		
		if(DEBUG_0){
		cout<<"EOP while 'i' value: "<<i<<std::endl;
		}
		
		strncpy(&error_detect_char[i], &buffer[i], 1);
		i++;
	}

	if(DEBUG_0){
		cout<<"Parsed EOP...\n";
	}

	place =i;
	
	if(DEBUG_0){
	cout<<"Value i: "<<i<<"\nPlace: "<<place<<std::endl;
	}
	/*
	int x = 0;

	    while(x < BUFFSIZE){
	    cout<<"Buffer["<<x<<"]: "<<buffer[x]<<" ascii val: "<<(int) buffer[x]<<std::endl;
	    x++;
		}
	*/
	if(DEBUG_0){
		cout<<"Parsing usable bytes...\n";
	}

	while (i < place+USABLE_BYTES){
		
		error_detect_char[i] = buffer[i];
		i++;
	}

	//x= 0;
	//while(x < startERRD){
	//cout<<"error detect byte by byte: "<<error_detect_char[x]<<std::endl;
	//x++;
	//}
	
	//usable_b[USABLE_BYTES] = '\0';
	if(DEBUG_0){
		cout<<"Printing usable_b string: "<<error_detect_char<<std::endl;
		cout<<"Parsed usable bytes...\n";
	}
	
	place=i;
	
	if(DEBUG_0){
		cout<<"Parsing payload...\n";
	}
	

	while (i < place+PAYLOAD_SIZE){
		
		if(DEBUG_0){
			cout<<"Value i: "<<i<<std::endl;
		}

		//cout<<"Copying "<<buffer[i]<<" to error_detect_char["<<i<<"]\n\n";
		
		error_detect_char[i] = buffer[i];

		//cout<<"error_detect_char["<<i<<"] is now"<<error_detect_char[i]<<std::endl;
		i++;
		
		if(DEBUG_0){
		cout<<"Value i: "<<i<<std::endl;
		cout<<"Payload["<<i-startPayload<<"] "<<&payload[i-startPayload]<<std::endl;
		}
	}
	//memset(&new_frame->payload[bytes_rcvd], 0, PAYLOAD_SIZE - bytes_rcvd);
	if(DEBUG_0){	
		cout<<"Parsed payload...\n";
	}
	//return error_detect_char;
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

void initial_print(){
	cout<<"\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\nXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n\n THE PROGRAM STARTS HERE \n\nXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n";
}