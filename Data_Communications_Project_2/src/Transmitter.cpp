#include <iostream>
#include <fcntl.h>
#include <string.h>
#include <netdb.h> 

typedef uint16_t crc;

#define POLYNOMIAL 0x8005;
#define WIDTH (8 * sizeof(crc))
#define MSB (1 << (WIDTH - 1))

using namespace std;

const int MAX_FRAME_SIZE = 64;
const int SYN = 22;
const int SIZE_OF_HEADER = 4;
int maxNumberOfErrorsInFrame;

int errorDetectionCorrectionType = 0;

//this is the structure that opens the frame
struct frame
{
	char header[SIZE_OF_HEADER];
	char frame[MAX_FRAME_SIZE];
};

//error for socket stuff
void error(const char *msg)
{
    perror(msg);
    exit(0);
}

//Here we append an appropriate parity bit for
//EVEN parity for the char passed to it.
char appendParityBit(char inputChar)
{
	int count = 1;
	char tempChar = inputChar;

	while (tempChar)
	{
		count ^= (tempChar & 1);
		tempChar >>= 1;
	}

	if (count)
	{
		inputChar |= (1 << 7);
	}

	return inputChar;
}

//We pass a frame here and append parity to
//the frame and header
void appendParityToFrame(frame *&myFrame)
{
	int size = (int) myFrame->header[SIZE_OF_HEADER - 1];

	for (int i = 0; i < SIZE_OF_HEADER; i++)
		myFrame->header[i] = appendParityBit(myFrame->header[i]);
	for (int i = 0; i < size; i++)
		myFrame->frame[i] = appendParityBit(myFrame->frame[i]);
}

//Here we calculate the CRC remainder
//and append it to the end of the 
//message.
void appendCRC(char myFrame[], int messageSize)
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
	myFrame[messageSize] = (remainder >> 8);
	myFrame[messageSize + 1] = remainder;
}

//This will transmit the character over
//the socket.
void transmitByte(char val, int sockfd)
{
	for (int b = 0; b < 8; b++) 
	{
		char item = (char) (val & 1) + '0';

   		int n = write(sockfd,&item,1);
   		if (n < 0) 
   			error("ERROR writing to socket");
		val >>= 1;	
	}
}

//This generates an array of check bits
//for the given byte.
char *generateCheckBits(char val)
{
	char *cBits = new char[4];

	cBits[3] = 1;
	cBits[3] ^= ((val >> 3) & 1) ^ 
				((val >> 2) & 1) ^ 
				((val >> 1) & 1) ^ 
				((val >> 0) & 1);

	cBits[2] = 1;
	cBits[2] ^= ((val >> 6) & 1) ^ 
				((val >> 5) & 1) ^ 
				((val >> 4) & 1) ^ 
				((val >> 0) & 1);

	cBits[1] = 1;
	cBits[1] ^= ((val >> 7) & 1) ^ 
				((val >> 5) & 1) ^ 
				((val >> 4) & 1) ^ 
				((val >> 2) & 1) ^ 
				((val >> 1) & 1);

	cBits[0] = 1;
	cBits[0] ^= ((val >> 7) & 1) ^ 
				((val >> 6) & 1) ^ 
				((val >> 4) & 1) ^ 
				((val >> 3) & 1) ^ 
				((val >> 1) & 1);

	return cBits;
}

//This transmits each check bit over 
//the socket.
void transmitCheckBits(int sockfd, char *cBits)
{
	char charToWrite = cBits[0];
	int n = write(sockfd,&charToWrite,1);
   	if (n < 0) 
   		error("ERROR writing to socket");
	charToWrite = cBits[1];
	n = write(sockfd,&charToWrite,1);
   	if (n < 0) 
   		error("ERROR writing to socket");
	charToWrite = cBits[2];
	n = write(sockfd,&charToWrite,1);
   	if (n < 0) 
   		error("ERROR writing to socket");
	charToWrite = cBits[3];
	n = write(sockfd,&charToWrite,1);
   	if (n < 0) 
   		error("ERROR writing to socket");
}

//Creates a random number of errors dictated by
//MAX_NUMBER_OF_ERRORS_IN_FRAME and distributes
//them randomly through the frame.  It also
//outputs the byte and bit number of the error.
void errorCreationModule(char frame[], int size)
{
	int numberOfErrorsInFrame = rand() % (maxNumberOfErrorsInFrame + 1);
	
	cout << numberOfErrorsInFrame;
	cout << " total errors";
	if (numberOfErrorsInFrame != 0)
	{
		cout << endl;
		cout << "Locations: ";
	}
	cout << endl;

	for (int i = 0; i < numberOfErrorsInFrame; i++)
	{
		int byteNumber = (rand() % size);	
		int bitNumber = (rand() % 8);

		cout << "byte #";
		cout << byteNumber;
		cout << " bit #";
		cout << bitNumber;
		cout << ", ";
		cout << endl;

		frame[byteNumber] = frame[byteNumber] ^ (1 << bitNumber);	
	}
	cout << endl;
}

//Here we start by appending parity to each item of the frame.
//Then depending on the error detection/correction type
//we can append the CRC to the frame or generate checkbits
//for the hamming check.  We then run the frame through the 
//error creation module.  Finally, we transmit the byte.
void transmitFrame(frame *&myFrame, int sockfd, int messageSize)
{
	int size = (int) myFrame->header[SIZE_OF_HEADER - 1];

	appendParityToFrame(myFrame);

	if (errorDetectionCorrectionType == 1)
	{
		appendCRC(myFrame->frame, messageSize);
	}

	char **checkBitsForFrame = new char*[size];

	if (errorDetectionCorrectionType == 2)
	{
		for (int i = 0; i < size; i++)
		{
			checkBitsForFrame[i] = generateCheckBits(myFrame->frame[i]);
		}
	}

	errorCreationModule(myFrame->frame, size);

	for (int i = 0; i < SIZE_OF_HEADER; i++)
	{
		transmitByte(myFrame->header[i], sockfd);
	}

	if (errorDetectionCorrectionType == 2)
	{
		for (int i = 0; i < size; i++)
		{
			transmitByte(myFrame->frame[i], sockfd);

			transmitCheckBits(sockfd, checkBitsForFrame[i]);

			delete checkBitsForFrame[i];
		}
		delete checkBitsForFrame;
	}
	else
	{
		for (int i = 0; i < size; i++)
		{
			transmitByte(myFrame->frame[i], sockfd);
		}
	}
}

//here we setup the socket and return the file descriptor
//we wil. assume port no 5050 and localhost.
int setupSocket()
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    portno = 5050;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    server = gethostbyname("localhost");
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");
	return sockfd;
}

//here we construct the frames.  We add 2 SYN characters 
//to the header as well as the frame size and then
//construct the data portion of the frame.  after the 
//frame is constructed we will transmit it right away.
void constructFrames(char *&buffer, int &bufferSize)
{
	int size = bufferSize;
	int count = 0;
	int sockfd = setupSocket();

	int messageSize = MAX_FRAME_SIZE;
	int frameSize = MAX_FRAME_SIZE;

	if (errorDetectionCorrectionType == 1)
	{
		messageSize = MAX_FRAME_SIZE - 2;	
	}

	int frameNumber = 1;

	while (size != 0)
	{
		cout << "Frame #";
		cout << frameNumber;
		cout << ": ";

		if (size < messageSize)
		{
			messageSize = size;
			frameSize = messageSize;

			if (errorDetectionCorrectionType == 1)
			{
				frameSize = messageSize + 2;		
			}
		}

		frame *myFrame = new frame[frameSize + 3];

		myFrame->header[0] = SYN;
		myFrame->header[1] = SYN;
		myFrame->header[2] = errorDetectionCorrectionType;
		myFrame->header[3] = frameSize;

		for (int i = 0; i < messageSize; i++)
		{
			myFrame->frame[i] = buffer[i + count];
		}

		count += messageSize;

		transmitFrame(myFrame, sockfd, messageSize);
		size -= messageSize;
		frameNumber++;	
	}
	close(sockfd);
}

//here we handle opening the file that is to be transmitted.
//we ask for a file name, open the file descriptor, and make
//sure that the file descriptor is valid. We also
//get a valid error correction/detection type.
void getInput(char *&buffer, int &bufferSize)
{
	string fileName;
	int myfd; 

	while(true)
	{
		cout << "Please enter the name of the file that contains the data you want to transmit: ";
		cin >> fileName;
		
		myfd = open(fileName.c_str(), O_RDONLY);
		if(myfd >= 0) break;
		cout << "You entered an invalid file name!" << endl;
	}

	while (true)
	{
		cout << "Please enter the error correction/detection" << endl;
		cout << "type you wish to use.  (0 for none, 1 for CRC, 2 for Hamming): ";

		string errorType;

		cin >> errorType;

		const char *cErrorType = errorType.c_str();

		if (strlen(cErrorType) > 1 || strlen(cErrorType) < 1)
		{
			cout << "invalid selection try again" << endl;
			continue;
		}

		errorDetectionCorrectionType = cErrorType[0] - '0';		

		if (errorDetectionCorrectionType == 0 || errorDetectionCorrectionType == 1 || errorDetectionCorrectionType == 2)
		{
			break;
		}
		
		cout << "invalid selection try again" << endl;
	}

	cout << "What is the maximum number of errors you would" << endl;
	cout << "like to have per frame?: ";
	
	cin >> maxNumberOfErrorsInFrame;

	int fileSize = lseek(myfd, 0, SEEK_END);
	buffer = new char[fileSize]; 
	lseek(myfd, 0, SEEK_SET);
	bufferSize = (int) read(myfd, buffer, fileSize);
	close(myfd);
}

int main()
{
	char *buffer;
	int bufferSize;

	getInput(buffer, bufferSize);	
	constructFrames(buffer, bufferSize);
}

