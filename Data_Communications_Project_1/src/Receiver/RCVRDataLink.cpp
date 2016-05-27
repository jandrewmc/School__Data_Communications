
#include <iostream>
#include <string.h>
#include <netinet/in.h>
#include <sys/poll.h>

#include "RCVRApplication.h"

using namespace std;

//handle error in socket construction
static void error(const char *msg)
{
    perror(msg);
    exit(1);
}

//here we take the created character and check the parity.  if the 
//parity matches, we return the character with the parity removed, if parity doesn't pass
//we return a 0 character and alert the user to an error in that bit character
char checkAndRemoveParity(char convertedChar)
{
	int count = 0;
	unsigned char value = convertedChar;

	while (value)
	{
		count ^= (value & 1);
		value >>= 1;
	}
	
	if (count)
	{
		convertedChar <<= 1;
		convertedChar = (unsigned char) convertedChar >> 1;
		return convertedChar;
	}
	else
	{
		cout << "error";
		return '0';
	}	
}

//this method takes in a pointer to an array of characters and
//convertes them to a single character.
char convertToChar(char *inputChar)
{
	char value;
	for (int i = 0; i < 8; i++)
	{
		int num = (int)(inputChar[i] - '0');

		value <<= 1;
		value |= num;
	}
	return value;
}

//here we use the file descriptor and input a single character at a time
//then we convert 8 of said characters into a character array and then convert said array to a single 
//character and then
//check the parity on said character.
char getNextChar(int myfd, int &errorVal)
{
	char *inputChar = new char[8];
	struct pollfd pol;

	pol.fd = myfd;
	pol.events = POLLIN;

	poll(&pol, 1, -1);

	for (int i = 7; i >=0; i--)
	{
		int n = read(myfd, &inputChar[i], 1); 
     	if (n < 0) error("ERROR reading from socket");
		else if (n == 0)
		{
			errorVal = -1; 
			return '0';
		}
	}	

	char convertedChar = convertToChar(inputChar);
	delete inputChar;
	return checkAndRemoveParity(convertedChar);
}

//For deframing the data sent over the socket, we wait for 2 SYN characters and a controll character
//then we receive the data.  Each character is sent over as 8 binary characters that represent
//the binary of the character.  These 8 symbols need to be converted to a char before processed.
int deframe(int myfd)
{
	int errorVal = 0;
	int syn1 = (int) getNextChar(myfd, errorVal);	
	if(errorVal < 0)
		return -1;

	if (syn1 == 22)
	{
		int syn2 = (int) getNextChar(myfd, errorVal);
		if (errorVal < 0)
			return -1;
		if (syn2 == 22)
		{
			int bufferSize = (int) getNextChar(myfd, errorVal);
			if (errorVal < 0)
				return -1;	
			for (int i = 0; i < bufferSize; i++)
			{
				char val = getNextChar(myfd, errorVal);
				if (errorVal < 0)
					return -1;

				if ((int) val == 10) break;

				outputReceivedData(val);
			}
			bufferSize = 0;
		}
		syn2 = 0;
	}
	syn1 = 0;
	return 0;
}
