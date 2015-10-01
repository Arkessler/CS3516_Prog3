/*
Author: Alexi Kessler
Code originally from TCP/IP Sockets in C by Donahoo and Calvert, modified for use
8/30/2015
*/
#include <stdio.h>
#include <stdlib.h>

void DieWithError(char *errorMessage)
{
	perror(errorMessage);  //Print error
	exit(1);
}

