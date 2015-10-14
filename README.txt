Important topics

-Input photo format
	-IMPORTANT
	-The program expects client id's that are four-digits long. The photo number is one character long.
	-This means that photo names must of a similar format to photo19911.jpg with 1991 being the client id and 1 being the photo number
-Inclusion of data length in frame
	-We have chosen to add a data length field to our frames. This is to allow for frames that do not contain full payloads.
-Padding of frame fields and ACKS
	-We have chosen to pad both our frames and ACKS to make them constant length. This means that a payload containing only a single character 
	would be 130 characters long, consisting of the single character and 129 0's. This is to make our received strings universally the same size
	and thus allow for easier processing.

Client specific
-Client usage
	-Proper usage of the client is described via output, but in short, the proper command is 
	./Client hostname (four-digit clientid) (two-digit number of photos)
-Chose not to use nwl_recv(), instead had dll_send return a frame
	-This was an implementation choice that was made possible because the network layer sends only a single packet at a time.
-Frames sent counts retransmissions
	-We were unsure of whether or not to count retransmitted frames in the total number of frames sent, we decided to do so.
-Expects First photo to be photoCid0.jpg
	-The client expects a naming schema where the first photo is numbered 0.

Server specific

-Server usage
	-Using DEBUG values:
		The program has multiple definitions of the constant DEBUG (DEBUG_0, DEBUG_1, DEBUG_2). This allows us to select what functions want to be debbug specifically. 
		The setup is the following:
			DEBUG_0 -> read_frame();
			DEBUG_1 -> get_init_pck();
			DEBUG_2 -> build_packet();
			DEBUG_3 -> build_frame_ack();
			DEBUG_4 -> build_packet_ack();
			DEBUG_5 -> check_error();
			DEBUG_SPECIAL -> Used to print the value of the error detecting string within the Phl recieve
			 
		The constant ACK_CORRUPTION is defined to run a piece of code that will send a bad ACK frame every 11th frame. We could not get this working (explained bellow) but left the code in there so that it can be tested


	-No ACK Corruption
		When the frame that receives a bad ACK is the last frame in packet, the issue is that server sends nwl_ACK, which the client, while trying to retransmit the previous frame, receives as a premature packet ACK, and breaks.

	-Handling multiple clients
		The program was tested to handle multiple clients simultaniously and it does support more than one connection a time. The parent function is always listening for incoming connection while the child processes take care of the rest of the routine.

	-One byte off issue
		We were having a problem where our copied picture would have a random extra byte every 256 bytes. Our function that builds the packet, build_packet(), was copying an extra byte into the buffer and therefore we decided to decrease by one byte the amount of usable data in the packet payload. After this simple solution, the program worked perfectly. 