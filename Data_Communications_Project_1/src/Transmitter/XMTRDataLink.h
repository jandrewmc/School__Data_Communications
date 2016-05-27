#ifndef XMTRDATALINK_H
#define XMTRDATALINK_H

const int MAX_FRAME_SIZE = 64;
const int SYN = 22;

void constructFrames(char *&buffer, int &bufferSize);

//this is the structure that opens the frame
typedef struct
{
	char header[3];
	char frame[MAX_FRAME_SIZE];
}frame;


#endif
