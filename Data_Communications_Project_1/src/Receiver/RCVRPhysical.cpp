
#include <iostream>
#include <string.h>
#include <netinet/in.h>
#include <sys/poll.h>

#include "RCVRDataLink.h"

using namespace std;

//handle error in socket construction
static void error(const char *msg)
{
    perror(msg);
    exit(1);
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
