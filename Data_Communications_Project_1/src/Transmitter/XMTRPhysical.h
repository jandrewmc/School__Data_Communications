#ifndef XMTRPHYSICAL_H
#define XMTRPHYSICAL_H

#include "XMTRDataLink.h"

typedef uint16_t crc;

void transmitFrame(frame &myFrame, int sockfd, int messageSize);
void error(const char *msg);

#endif
