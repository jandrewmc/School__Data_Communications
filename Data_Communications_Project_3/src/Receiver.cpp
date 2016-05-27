
//*************************************************************
//Initial Items Start
//*************************************************************

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

int deframe(int myfd, char **&frameBuffer, int frameSizeBuffer[], int &lastFrameReceived);
void printFrame(char frame[], int size);
void printCRCFrame(char frame[], int size);

//*************************************************************
//Initial Items End
//*************************************************************

//*************************************************************
//Physical Layer Start
//*************************************************************

//This method divides by the CRC polynomial. 
//If the remainder is 0, the CRC passes.
crc doesCRCFail(char myFrame[], int messageSize)
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
//	delete inputChar;
	return convertedChar;
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

//This method handles receiving the message that
//is transmitted and handles checking CRC
void handleMessage(int myfd, int bufferSize, int sequenceNumber, char **&frameBuffer, int frameSizeBuffer[])
{
	int errorVal = 0;
	char *message = new char[bufferSize];
	char val;

	for (int i = 0; i < bufferSize; i++)
	{
		val = getNextChar(myfd, errorVal);

		if (errorVal < 0)
			break;

		message[i] = val;
	}

	if (doesCRCFail(message, bufferSize))
	{
//		delete [] message;
		return;
	}

	frameBuffer[sequenceNumber] = message;
	frameSizeBuffer[sequenceNumber] = bufferSize;
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

	char **frameBuffer = new char*[8];
	int frameSizeBuffer[8];
	
	int windowLowBound = 0;
	int windowHighBound = 3;
	int lastFrameReceived = 7;
	int lastFrameAcknowledged = 7;

	deframe(newsockfd, frameBuffer, frameSizeBuffer, lastFrameReceived);

	//if we receive a frame and it is within the window (not a SREJ frame), then
	//we are going to modify the window low bound accordingly
	int windowRange = windowHighBound - windowLowBound + 1;
	if (windowRange < 0) windowRange += 8;
	int tempWindowLowBound = windowLowBound;
	while (windowRange > 0)
	{
		if (tempWindowLowBound == lastFrameReceived)
		{
			windowLowBound = (lastFrameReceived + 1) % 8;
				
			break;
		}
		windowRange--;
		tempWindowLowBound = (tempWindowLowBound + 1) % 8;
	}

	while (true)
	{
		cout << lastFrameReceived << endl;
		cout << lastFrameAcknowledged << endl;
		cout << windowLowBound << endl;
		cout << windowHighBound << endl;
		struct pollfd pol;
		pol.fd = sockfd;
		pol.events = POLLIN;

		poll(&pol, 1, 10000); //poll on the socket with a 1 second time out

		if (pol.revents & POLLIN) //if there is data to read on the socket
		{
			deframe(newsockfd, frameBuffer, frameSizeBuffer, lastFrameReceived);

			int windowRange = windowHighBound - windowLowBound + 1;
			if (windowRange < 0) windowRange += 8;
			int tempWindowLowBound = windowLowBound;
			while (windowRange > 0)
			{
				if (tempWindowLowBound == lastFrameReceived)
				{
					windowLowBound = (lastFrameReceived + 1) % 8;
						
					break;
				}
				windowRange--;
				tempWindowLowBound = (tempWindowLowBound + 1) % 8;
			}
		}
		else //if there is no data to read on the socket
		{
			//do an acknowledgement sequence
			int val = (lastFrameAcknowledged + 1) % 8;
			int count = 0;

			while (frameBuffer[val] != nullptr) //if the frame was received and good
			{
				printCRCFrame(frameBuffer[val], frameSizeBuffer[val]); //print it
				frameBuffer[val] = nullptr; //delete it
				frameSizeBuffer[val] = 0; 
				val = (val + 1) % 8;  //and go on to check the next value
				count++; //keep track of how many frames are handled this way
			}

			if(count == 0) //if the first element in the buffer is null
			{
				char SREJ = 21;
				int n = write(newsockfd, &SREJ, 1);
				if (n < 0)
					error("Error writing to socket");
				n = write(newsockfd, &val, 1);
				if (n < 0)
					error("Error writing to socket");
			}
			else //send ACK for the next expected frame
			{
				char ACK = 6;
				int n = write(newsockfd, &ACK, 1);
				if (n < 0)
					error("Error writing to socket");
				n = write(newsockfd, &val, 1);
				if (n < 0)
					error("Error writing to socket");

				int numberOfFramesAcknowledged = val - lastFrameAcknowledged + 7;
				int count = numberOfFramesAcknowledged;
				while (count > 0)
				{
					frameBuffer[(lastFrameAcknowledged + 1) % 8] = nullptr;
					count--;
				}
				lastFrameAcknowledged = (lastFrameAcknowledged + numberOfFramesAcknowledged) % 8;
				windowHighBound = (windowHighBound + numberOfFramesAcknowledged) % 8;

				cout << endl;
				cout << endl;
				cout << "ACK ";
				cout << val << endl;
				cout << endl;
				
			}

		}
	}

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

//*************************************************************
//Physical Layer End
//*************************************************************

//*************************************************************
//Data Link Layer Start
//*************************************************************

//Here we take the file descriptor, we wait for two SYN
//characters, get the error detection/correction type,
//message size, and then get the message.
int deframe(int myfd, char **&frameBuffer, int frameSizeBuffer[], int &lastFrameReceived)
{
	int syn = getHeaderChar(myfd);	
	if (syn < 0) return -1;

	if (syn == 22)
	{
		syn = getHeaderChar(myfd);
		if (syn < 0) return -1;

		if (syn == 22)
		{
			int sequenceNumber = getHeaderChar(myfd);
			if (sequenceNumber < 0) return -1;

			int bufferSize = getHeaderChar(myfd);
			if (bufferSize < 0) return -1;

			handleMessage(myfd, bufferSize, sequenceNumber, frameBuffer, frameSizeBuffer);	
			lastFrameReceived = sequenceNumber;
		}
	}
	return 0;
}

//*************************************************************
//Data Link Layer End
//*************************************************************

//*************************************************************
//Application Layer Start
//*************************************************************

//this method will print out the resultant
//frame that was transmitted with CRC
void printCRCFrame(char frame[], int size)
{
	for (int i = 0; i < size - 2; i++)
	{
		cout << checkAndRemoveParity(frame[i]);
	}
}

//*************************************************************
//Application Layer End
//*************************************************************
