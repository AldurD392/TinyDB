#ifndef __LOGGER_H__
#define __LOGGER_H__
#include <stdio.h>

#ifndef _WIN32
#include <netinet/in.h>
#else
#include <winsock2.h>
#include <Ws2tcpip.h>
#endif
#include "Protocol.h"

void logAppend(char* logpath, int code, struct sockaddr_storage *managedSocketAddress, int size, struct url *url);

#endif
