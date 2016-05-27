#include <iostream>
#include <string.h>
#include <netinet/in.h>
#include <sys/poll.h>

typedef uint16_t crc;

#define POLYNOMIAL 0x8005
#define WIDTH (8 * sizeof(crc))
#define MSB (1 << (WIDTH - 1))

using namespace std;

int frameNumber = 1;

//handle error in socket construction
void error(const char *msg)
{
    perror(msg);
    exit(1);
}

//This method divides by the CRC polynomial. 
//If the remainder is 0, the CRC passes.
crc checkCRC(char myFrame[], int messageSize)
{
	crc remainder = 0;

	for (int byte = 0; byte < messageSize; byte++)
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
	return remainder;
}

//This method will check the parity on the characters
//transmitted.  If parity passes, remove the parity 
//bit.  If parity fails, output #PCF#
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
		cout << "#PCF";
		return '#';
	}	
}

//This method takes the input c-string
//and converts it into a char type
char convertToChar(char *inputChar)
{
	char result;
	for (int i = 0; i < 8; i++)
	{
		int num = (int)(inputChar[i] - '0');

		result <<= 1;
		result |= num;
	}
	return result;
}

//Here, using the file descriptor we take in
//8 bits that will become a single character
//and return the data as a character
char getNextChar(int myfd, int &errorVal)
{
	char *inputChar = new char[8];
	struct pollfd pol;

	pol.fd = myfd;
	pol.events = POLLIN;

	poll(&pol, 1, -1);

	for (int i = 7; i >= 0; i--)
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
	return convertedChar;
}

//If the syndrome is non-zero, we use the calculated value to 
//flip the appropriate bit in the transmitted character
char correctCharacter(char value, int syndrome, int currentByte)
{
	int bitToChange = -1;
	if (syndrome >= 9 && syndrome <= 12)
	{
		bitToChange = syndrome - 12;
		if (bitToChange < 0)
			bitToChange *= -1;
		value ^= (1 << bitToChange);
	}
	else if (syndrome >= 5 && syndrome <= 7)
	{
		bitToChange = syndrome - 11;
		if (bitToChange < 0)
			bitToChange *= -1;
		value ^= (1 << bitToChange);
	}
	else if (syndrome == 3)
	{
		bitToChange = syndrome - 10;
		if (bitToChange < 0)
			bitToChange *= -1;
		value ^= (1 << bitToChange);
	}

	if (bitToChange != -1)
	{
		cout << "byte #";
		cout << currentByte;
		cout << ", bit #";
		cout << bitToChange;
		cout << ", error found and corrected!" << endl;;
	}

	return value;
}

//Here we take the check bits that are transmitted with 
//the byte and compare them to check bits that are
//calculated from the received message.
int generateSyndrome(char *cBits, char val, int currentByte)
{
	int cBit4 = 1;
	cBit4 ^= 	((val >> 3) & 1) ^ 
				((val >> 2) & 1) ^ 
				((val >> 1) & 1) ^ 
				((val >> 0) & 1);

	int cBit3 = 1;
	cBit3 ^= 	((val >> 6) & 1) ^ 
				((val >> 5) & 1) ^ 
				((val >> 4) & 1) ^ 
				((val >> 0) & 1);

	int cBit2 = 1;
	cBit2 ^=	((val >> 7) & 1) ^ 
				((val >> 5) & 1) ^ 
				((val >> 4) & 1) ^ 
				((val >> 2) & 1) ^ 
				((val >> 1) & 1);

	int cBit1 = 1;
	cBit1 ^= 	((val >> 7) & 1) ^ 
				((val >> 6) & 1) ^ 
				((val >> 4) & 1) ^ 
				((val >> 3) & 1) ^ 
				((val >> 1) & 1);

	int syndrome = 0;

	syndrome ^= cBit4 ^ (int)cBits[3];
	syndrome <<= 1;
	syndrome ^= cBit3 ^ (int)cBits[2];
	syndrome <<= 1;
	syndrome ^= cBit2 ^ (int)cBits[1];
	syndrome <<= 1;
	syndrome ^= cBit1 ^ (int)cBits[0];

	return syndrome;
}

//Here we get the next hamming character meaning we receive
//the first 8 bits that make up the character transmitted
//and then we receive the next 4 bits that make up the 
//check bits over that character
char getNextHammingChar(int myfd, int &errorVal, int currentByte)
{
	char *inputChar = new char[8];

	struct pollfd pol;
	pol.fd = myfd;
	pol.events = POLLIN;
	poll(&pol, 1, -1);

	//get the transmitted character
	for (int i = 7; i >= 0; i--)
	{
		int n = read(myfd, &inputChar[i], 1); 
     	if (n < 0) error("ERROR reading from socket");
		else if (n == 0)
		{
			errorVal = -1; 
			return '0';
		}
	}	

	char val = convertToChar(inputChar);

	//get the check bits for the byte
	char *cBits = new char[4];
	for (int i = 0; i < 4; i++)
	{
		read(myfd, &cBits[i], 1);
	}

	//perform hamming
	int syndrome = generateSyndrome(cBits, val, currentByte);
	if (syndrome != 0)
	{
		val = correctCharacter(val, syndrome, currentByte);
	}

	delete inputChar;
	delete cBits;

	return val;	
}

//Get a character that is expected to be from the 
//header of the frame, remove parity, and 
//return the value.
int getHeaderChar(int fd)
{
	int errorVal = 0;
	char headerItem = getNextChar(fd, errorVal);
	if (errorVal < 0) return -1;
	return (int) checkAndRemoveParity(headerItem);
}

//this method will print out the resultant frame
//that is transmitted.
void printFrame(char frame[], int size)
{
	for (int i = 0; i < size; i++)
	{
		cout << checkAndRemoveParity(frame[i]);
	}
	cout << endl;
	cout << endl;
}

//this method will print out the resultant
//frame that was transmitted with CRC
void printCRCFrame(char frame[], int size)
{
	for (int i = 0; i < size - 2; i++)
	{
		cout << checkAndRemoveParity(frame[i]);
	}
	cout << endl;
	cout << endl;
}

//This method handles receiving the message that
//is transmitted and handles checking CRC
void handleMessage(int myfd, int bufferSize, int errorDetectionCorrectionType, int currentFrame)
{
	cout << "Frame #";
	cout << frameNumber;
	cout << ":" << endl;

	int errorVal = 0;
	char message[bufferSize];
	char val;

	for (int i = 0; i < bufferSize; i++)
	{
		if (errorDetectionCorrectionType == 2)
		{
			val = getNextHammingChar(myfd, errorVal, i);	
		}
		else
		{
			val = getNextChar(myfd, errorVal);
		}

		if (errorVal < 0)
			break;

		message[i] = val;
	}

	switch(errorDetectionCorrectionType)
	{
		case 0:
			printFrame(message, bufferSize);
			break;
		case 1:
			if (checkCRC(message, bufferSize))
			{
				cout << "CRC failed" << endl;
				cout << endl;
			}
			else
			{
				printCRCFrame(message, bufferSize);
			}
			break;
		case 2:
			printFrame(message, bufferSize);
			break;
		default:
			cout << "error, failed to get a valid error correction/detection type" << endl;
	}
}

//Here we take the file descriptor, we wait for two SYN
//characters, get the error detection/correction type,
//message size, and then get the message.
int deframe(int myfd)
{
	int syn = getHeaderChar(myfd);	
	if (syn < 0) return -1;

	if (syn == 22)
	{
		syn = getHeaderChar(myfd);
		if (syn < 0) return -1;

		if (syn == 22)
		{
			int errorDetectionCorrectionType = getHeaderChar(myfd);
			if (errorDetectionCorrectionType < 0) return -1;

			int bufferSize = getHeaderChar(myfd);
			if (bufferSize < 0) return -1;

			handleMessage(myfd, bufferSize, errorDetectionCorrectionType, frameNumber);	
			frameNumber++;
		}
	}
	return 0;
}

//this method sets up the socket and handles the calling of deframe
void getInput(char *&buffer, int &bufferSize)
{
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
       error("ERROR opening socket");
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = 5050;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &serv_addr,
             sizeof(serv_addr)) < 0) 
             error("ERROR on binding");
    listen(sockfd,5);
    clilen = sizeof(cli_addr);

  	newsockfd = accept(sockfd, 
   		(struct sockaddr *) &cli_addr, 
   			&clilen);

   	if (newsockfd < 0) 
   	  error("ERROR on accept");

	while(deframe(newsockfd) >= 0)
		;

    close(newsockfd);
    close(sockfd);
}

int main()
{
	char *buffer;
	int bufferSize;

	getInput(buffer, bufferSize);
	cout << endl;
}

