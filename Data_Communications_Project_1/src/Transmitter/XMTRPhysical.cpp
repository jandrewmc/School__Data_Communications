#include <iostream>
#include <fcntl.h>
#include <netdb.h> 

#include "XMTRPhysical.h"

#define POLYNOMIAL 0x8005
#define WIDTH (8 * sizeof(crc))
#define MSB (1 << (WIDTH - 1))

//error for socket stuff
void error(const char *msg)
{
    perror(msg);
    exit(0);
}

//for each character we transmit, we need to add parity to it.
//this method counts the number of 1's in the character, then
//appends parity accordingly
char appendParityBit(char inputChar)
{
	int count = 1;
	char value = inputChar;

	while (value)
	{
		count ^= (value & 1);
		value >>= 1;
	}
	if (count)
	{
		inputChar |= (1 << 7);
	}
	return inputChar;
}

void appendCRC(char myFrame[], int sizeOfData)
{
	crc remainder = 0;

	for (int byte = 0; byte < sizeOfData; byte++)
	{
		remainder ^= (myFrame[byte] << (WIDTH - 8));

		for (uint8_t bit = 8; bit > 0; bit--)
		{
			if (remainder & MSB)
			{
				remainder = (remainder << 1) ^ POLYNOMIAL;
			}
			else
			{
				remainder = remainder << 1;
			}
		}
	}

	myFrame[sizeOfData] = (remainder >> 8);
	myFrame[sizeOfData + 1] = remainder;
}

//here we transmit the frame. first we transmit the frame
//header, then we transmit the data 
void transmitFrame(frame &myFrame, int sockfd, int messageSize)
{
	int size = (int) myFrame.header[2];
	//transmit frame header
	for (int i = 0; i < 3; i++)
	{
		char val = appendParityBit(myFrame.header[i]);
		for (int b = 0; b < 8; b++) 
		{
			char item = (char) (val & 1) + '0';
    		int n = write(sockfd,&item,1);
    		if (n < 0) 
        			error("ERROR writing to socket");
			val >>= 1;	
		}
	}

	//transmit frame data
	for (int i = 0; i < size; i++)
	{
		char val = appendParityBit(myFrame.frame[i]);
		for (int b = 0; b < 8; b++) 
		{
			char item = (char) (val & 1) + '0';
    		int n = write(sockfd,&item,1);
    		if (n < 0) 
        			error("ERROR writing to socket");
			val >>= 1;	
		}
	}
}
