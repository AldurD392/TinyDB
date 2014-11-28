#ifndef __Protocol_h__
#define __Protocol_h__

#include "Utils.h"

#include <stdlib.h>

#define BUFFER_SIZE 20
#define STARTING 7 // mydb://

#ifndef _WIN32
	#define CHARCAST
#else
	#define CHARCAST (char *)
#endif

struct acceptArgs {
    SOCKET_TYPE sfd;
    struct config* cfg;
};

struct url
{
	char* address;
	char* identifier;
	char* command;
	char* argument;
	char* optionalArgument;
};

void acceptOnSocket(struct acceptArgs *args);
int parseArgv(char* argv, struct url *url);
void* readAll(int sfd, size_t type, size_t* bufferSize);
int sendEntireFile(int sfd, char* path);
int recvSaveEntireFile(int sfd, char* path, char* mode);
#endif
