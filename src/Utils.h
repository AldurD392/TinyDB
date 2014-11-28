#ifndef __Utils_h__
#define __Utils_h__

#ifndef _WIN32

#else
	#include <winsock2.h>
	#include <ws2tcpip.h>
#endif

#ifdef _WIN32
	char *strsep(char **sp, char *sep);
#endif

#ifndef _WIN32
	#define SOCKET_TYPE int
#else
	#define SOCKET_TYPE SOCKET
#endif

void handle_error(char* error);
SOCKET_TYPE* createServerSocket(char* port, int* len);
SOCKET_TYPE createClientSocket(char* address, const char* port);
int checkFileExists(char* path);
int checkFolderExists(char* path);
void initWSA();
#endif
