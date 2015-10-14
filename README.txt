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
-No ACK Corruption
	-When frame that receives bad ACK is last frame in packet, issue is that server sends nwl_ACK, which the client, while trying
	to retransmit the previous frame, receives as a premature packet ACK, and breaks.