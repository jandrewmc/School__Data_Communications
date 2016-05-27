#include <iostream>
#include <fcntl.h>
#include <netdb.h> 

#include "XMTRDataLink.h"
#include "XMTRPhysical.h"

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
	int isCRC = 0;

	int size = bufferSize;
	int count = 0;
	int sockfd = setupSocket();

	int messageSize = 64;
	int frameSize = 64;

	if (isCRC)
	{
		messageSize = frameSize - sizeof(crc);
	}

	while (size != 0)
	{
		if (size < messageSize)
		{
			messageSize = size;
			frameSize = messageSize + sizeof(crc);
		}

		frame myFrame;

		myFrame.header[0] = SYN;
		myFrame.header[1] = SYN;
		myFrame.header[2] = frameSize;

		for (int i = 0; i < messageSize; i++)
		{
			myFrame.frame[i] = buffer[i + count];
		}

		count += messageSize;

		transmitFrame(myFrame, sockfd, messageSize);
		size -= messageSize;
	}
	std::cout << std::endl;
	close(sockfd);
}


