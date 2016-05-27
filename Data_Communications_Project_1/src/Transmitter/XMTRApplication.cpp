#include <iostream>
#include <string.h>
#include <fcntl.h>
#include <netdb.h> 

#include "XMTRDataLink.h"

using namespace std;

//here we handle opening the file that is to be transmitted.
//we ask for a file name, open the file descriptor, and make
//sure that the file descriptor is valid.
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

	int fileSize = lseek(myfd, 0, SEEK_END);
	buffer = new char[fileSize]; 
	lseek(myfd, 0, SEEK_SET);
	bufferSize = (int) read(myfd, buffer, fileSize);
	close(myfd);
}

//the main method handles getting the input and
//constructing the frames
int main()
{
	char *buffer;
	int bufferSize;

	bool shouldContinue = true;

	getInput(buffer, bufferSize);	
	constructFrames(buffer, bufferSize);
}
