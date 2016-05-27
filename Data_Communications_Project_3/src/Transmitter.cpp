
//*************************************************************
//Initial Items Start
//*************************************************************

#include <iostream>
#include <fcntl.h>
#include <string.h>
#include <netdb.h> 
#include <sys/poll.h>

typedef uint16_t crc;

#define POLYNOMIAL 0x8005;
#define WIDTH (8 * sizeof(crc))
#define MSB (1 << (WIDTH - 1))

using namespace std;

const int MAX_FRAME_SIZE = 64;
const int SYN = 22;
const int SIZE_OF_HEADER = 4;
int maxNumberOfErrorsInFrame;

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

//*************************************************************
//Initial Items End
//*************************************************************

//*************************************************************
//Error Creation Module Start
//*************************************************************

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

//*************************************************************
//Error Creation Module End
//*************************************************************

//*************************************************************
//Physical Layer Start
//*************************************************************

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

//Here we start by appending parity to each item of the frame.
//Then depending on the error detection/correction type
//we can append the CRC to the frame or generate checkbits
//for the hamming check.  We then run the frame through the 
//error creation module.  Finally, we transmit the byte.
void transmitFrame(frame *&myFrame, int sockfd, int messageSize)
{
	int size = (int) myFrame->header[SIZE_OF_HEADER - 1];

	appendParityToFrame(myFrame);
	appendCRC(myFrame->frame, messageSize);

	errorCreationModule(myFrame->frame, size);

	for (int i = 0; i < SIZE_OF_HEADER; i++)
	{
		transmitByte(myFrame->header[i], sockfd);
	}

	for (int i = 0; i < size; i++)
	{
		cout << myFrame->frame[i];
		transmitByte(myFrame->frame[i], sockfd);
	}
}

//*************************************************************
//Physical Layer End
//*************************************************************

//*************************************************************
//Data Link Layer Start
//*************************************************************

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

	int frameSize = MAX_FRAME_SIZE;
	int messageSize = frameSize - 2;

	int frameNumber = 0;
	frame *frameBuffer[8];

	//since the first frame to be transmitted is of sequence number 0, set both to 7
	int lastFrameAcknowledged = 7;
	int lastFrameTransmitted = 7;

	//set the window size to 4
	int windowLowBound = 0;
	int windowHighBound = 3;

	while (size != 0)
	{
		if (windowLowBound > windowHighBound) //if every frame that can be transmitted, is transmitted
		{
			struct pollfd pol;
			pol.fd = sockfd;
			pol.events = POLLIN;

			poll(&pol, 1, 1000); //poll on the socket with a 1 second time out

			if(pol.revents & POLLIN) //if there is data to receive on the socket
			{
				char *inputChar = new char[2];
				read(sockfd, &inputChar[0], 1);
				read(sockfd, &inputChar[1], 1);
				int nextExpectedFrame = (int) inputChar[1];

				if ((int)inputChar[0] == 6) //if the receiver sends back an ACK
				{
					int numberOfFramesAcknowledged = nextExpectedFrame - lastFrameAcknowledged + 7;
					int count = numberOfFramesAcknowledged;
					while (count > 0)
					{
						delete frameBuffer[(lastFrameAcknowledged + 1) % 8];
						count--;
					}
					lastFrameAcknowledged = (lastFrameAcknowledged + numberOfFramesAcknowledged) % 8;
					windowHighBound = (windowHighBound + numberOfFramesAcknowledged) % 8;
				}
				else if ((int)inputChar[0] == 21)
				{
					transmitFrame(frameBuffer[nextExpectedFrame], sockfd, (int)frameBuffer[nextExpectedFrame]->header[3] - 2);
				}
			}
			else //If the transmission times out, reset the window.
			{
				windowLowBound < 3 ? windowLowBound += 5 : windowLowBound -= 3; 
			}

			continue;
		}

		int frameSequenceNumber = frameNumber % 8;

		cout << "Frame #";
		cout << frameNumber;
		cout << ": ";

		if (size < messageSize)
		{
			messageSize = size;
			frameSize = messageSize + 2;		
		}

		frame *myFrame = new frame[frameSize + 3];
		frameBuffer[frameSequenceNumber] = myFrame;


		myFrame->header[0] = SYN;
		myFrame->header[1] = SYN;
		myFrame->header[2] = frameSequenceNumber;
		myFrame->header[3] = frameSize;

		for (int i = 0; i < messageSize; i++)
		{
			myFrame->frame[i] = buffer[i + count];
		}

		count += messageSize;

		transmitFrame(myFrame, sockfd, messageSize);
		size -= messageSize;

		frameNumber++;	
		windowLowBound = (windowLowBound + 1) % 8;
		lastFrameTransmitted = (lastFrameTransmitted + 1) % 8;

		struct pollfd pol;
		pol.fd = sockfd;
		pol.events = POLLIN;

		poll(&pol, 1, 0);

		if(pol.revents & POLLIN) //if there is data to receive on the socket
		{
			char *inputChar = new char[2];
			read(sockfd, &inputChar[0], 1);
			read(sockfd, &inputChar[1], 1);
			int nextExpectedFrame = (int) inputChar[1];

			if ((int)inputChar[0] == 6) //if the receiver sends back an ACK
			{
				int numberOfFramesAcknowledged = nextExpectedFrame - lastFrameAcknowledged + 7;
				int count = numberOfFramesAcknowledged;

				while (count > 0)
				{
					frameBuffer[(lastFrameAcknowledged + 1) % 8] = nullptr;
					count--;
				}
				lastFrameAcknowledged = (lastFrameAcknowledged + numberOfFramesAcknowledged) % 8;
				windowHighBound = (windowHighBound + numberOfFramesAcknowledged) % 8;
			}
			else if ((int)inputChar[0] == 21)
			{
				transmitFrame(frameBuffer[nextExpectedFrame], sockfd, (int)frameBuffer[nextExpectedFrame]->header[3] - 2);
			}
		}
	}

	//probably need to add something after all frames have been transmitted to make sure that 
	//final ack is sent
	close(sockfd);
}

//*************************************************************
//Data Link Layer End
//*************************************************************

//*************************************************************
//Application Layer Start
//*************************************************************

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

//*************************************************************
//Application Layer End
//*************************************************************

