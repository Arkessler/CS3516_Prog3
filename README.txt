Things worth describing here
-Proper usage of client and server
-Inclusion of data length in frame
-Padding of frame fields
-Send both frame ACK and nwl_ACK for final frame in packet ?


Client specific
-Chose not to use nwl_recv(), instead had dll_send return a frame
-Possible timeout issues with select() being called after phl_send

Server specific