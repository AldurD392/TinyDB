#include "Utils.h"
#include "Wrapper.h"
#include "Protocol.h"
#include "dbg.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <libgen.h>
#include <dirent.h>

#ifndef _WIN32
	#include <sys/socket.h>
	#include <arpa/inet.h>
	#include <netdb.h>
#else
	/* IPV6_V6ONLY is missing from pre-Windows 2008 SDK as well as MinGW
	 * (at least up through 1.0.16).
	 * Runtime support is a separate issue.
	 */
	#ifndef IPV6_V6ONLY
		#define IPV6_V6ONLY 27
	#endif

	// #if WINVER >= _WIN32_WINNT_VISTA  // Aggiunti a mano poiché non presenti in minGW per problemi di licenze
	#ifndef AI_NUMERICSERV
		#define AI_NUMERICSERV 0x00000008
	#endif
	// #endif

	#include <winsock2.h>
	#include <ws2tcpip.h>
#endif

#ifndef _WIN32
	#define SOCKET_ERROR_CODE -1
#else
	#define SOCKET_ERROR_CODE INVALID_SOCKET
#endif

#define EXIT_STATUS -1
#define BACKLOG 10

void handle_error(char* error) {
	log_err("%s", error);
	exit(EXIT_STATUS);
}

void initWSA() {
	#ifdef _WIN32
		/* Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h */
		#define WIN_SOCK_V MAKEWORD(2, 2)
		WSADATA wsaData;
		if (WSAStartup(WIN_SOCK_V, &wsaData) != 0) {
		    /* Tell the user that we could not find a usable */
		    /* Winsock DLL.                                  */
		    handle_error("WSAStartup failed");
		}

		/* Confirm that the WinSock DLL supports 2.2.*/
		/* Note that if the DLL supports versions greater    */
		/* than 2.2 in addition to 2.2, it will still return */
		/* 2.2 in wVersion since that is the version we      */
		/* requested.                                        */

	    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
	        /* Tell the user that we could not find a usable */
	        /* WinSock DLL.                                  */
	        if (WSACleanup() != 0) {
	        	handle_error("Error cleaning WSA");
	        }
	        handle_error("Could not find a usable version of Winsock library");
	    }
    #endif

	return;
}

SOCKET_TYPE* createServerSocket(char* port, int* len) {
	/* Questa funzione, dati indirizzo e numero di porta, crea un socket di tipo stream,
	per il protocollo inet, effettua la bind su indirizzo e porta, e si mette in ascolto. */

	struct addrinfo *addrInfoHints = Calloc(1, sizeof(struct addrinfo));
	struct addrinfo *addrInfos, *savedAddrInfos = NULL;

	addrInfoHints->ai_family 	= AF_UNSPEC;
	addrInfoHints->ai_socktype 	= SOCK_STREAM;
	addrInfoHints->ai_protocol 	= 0;
	addrInfoHints->ai_flags 	= AI_PASSIVE | AI_NUMERICSERV;
	addrInfoHints->ai_canonname = NULL;
	addrInfoHints->ai_addr 		= NULL;
	addrInfoHints->ai_addrlen 	= 0;
	addrInfoHints->ai_next 		= NULL;

	initWSA();  // Se eseguito in Unix, questa funzione non fa nulla, altrimenti inizializza il meccanismo di socket di Windows

	if (getaddrinfo(NULL, port, addrInfoHints, &addrInfos) != 0) {
		handle_error("Error getting address info");
	}
	savedAddrInfos = addrInfos;

	SOCKET_TYPE* sfds = NULL;
	int i = 0;
	do {
		SOCKET_TYPE sfd;
		if ((sfd = socket(addrInfos->ai_family, addrInfos->ai_socktype, addrInfos->ai_protocol)) == SOCKET_ERROR_CODE) {
			log_err("Error opening socket.");
		} else {
			// Setto il socket IPv6 per gestire soltanto le richieste IPv6 (in modo da non avere problemi di binding)
			if (addrInfos->ai_family == AF_INET6) {
				int optval = 1;
				if (setsockopt(sfd, IPPROTO_IPV6, IPV6_V6ONLY, CHARCAST &optval, sizeof(optval)) != 0) {
					handle_error("Error setting socket options");
				}
			}

			if (bind(sfd, addrInfos->ai_addr, addrInfos->ai_addrlen) != 0) {
				handle_error("Error binding socket");
			}

			if (listen(sfd, BACKLOG) != 0)	{
				handle_error("Error listening socket");
			}

			sfds = Realloc(sfds, sizeof(sfds) + sizeof(SOCKET_TYPE));
			sfds[i] = sfd;
			i++;
		}
	} while ((addrInfos = addrInfos->ai_next) != NULL);

	if (i == 0) {
		handle_error("Error creating sockets");
	}

	freeaddrinfo(savedAddrInfos);
	free(addrInfoHints);

	*len = i;
	return sfds;
}

SOCKET_TYPE createClientSocket(char* address, const char* port) {
	/* Questa funzione, dati un indirizzo e una porta, crea una socket per il client,
	inizializza la struttura di appoggio ed esegue la connect.
	*/

	struct addrinfo *addrInfoHints = Calloc(1, sizeof(struct addrinfo));
	struct addrinfo *addrInfos, *savedAddrInfos = NULL;

	addrInfoHints->ai_family 	= AF_UNSPEC;
	addrInfoHints->ai_socktype 	= SOCK_STREAM;
	addrInfoHints->ai_protocol 	= 0;
	addrInfoHints->ai_flags 	= AI_NUMERICSERV;
	addrInfoHints->ai_canonname = NULL;
	addrInfoHints->ai_addr 		= NULL;
	addrInfoHints->ai_addrlen 	= 0;
	addrInfoHints->ai_next 		= NULL;

	initWSA();  // Se eseguito in Unix, questa funzione non fa nulla, altrimenti inizializza il meccanismo di socket di Windows

	// Voglio collegarmi all'indirizzo address,
	// con la porta port
	// e voglio socket che siano di tipo IPv4 o IPv6
	int a;
	if ((a = getaddrinfo(address, port, addrInfoHints, &addrInfos)) != 0) {
		handle_error("Error getting address info");
	}
	savedAddrInfos = addrInfos;

	SOCKET_TYPE sfd = -1;
	do {
		if ((sfd = socket(addrInfos->ai_family, addrInfos->ai_socktype, addrInfos->ai_protocol)) == SOCKET_ERROR_CODE) {
			handle_error("Error opening socket");
		}

		// 3. Eseguo la connect.
		if (connect(sfd, addrInfos->ai_addr, addrInfos->ai_addrlen) != 0) {
			log_err("Connection error");
		} else {
			break;
		}

		close(sfd);
	} while ((addrInfos = addrInfos->ai_next) != NULL);
	if (addrInfos == NULL) {
		handle_error("Error while connecting to server");
	}

	freeaddrinfo(savedAddrInfos);
	free(addrInfoHints);

	return sfd;
}

int checkFileExists(char* path) {
	/* Questa funzione controlla l'esistenza del file regolare passato in path. */
	int returnCode = 1;

	FILE* file;
	if ((file = fopen(path, "r")) == NULL) {
		returnCode = 0;
	}

	fclose(file);
	return returnCode;
}

int checkFolderExists(char* path) {
	/* Questa funzione controlla l'esistenza della cartella passata in path. */
	int returnCode = 1;

	char* pathCopy = strdup(path);
	char* dir = dirname(pathCopy);  // Non è memoria allocata dinamicamente

	DIR* directoryStream = opendir(dir);
	if (directoryStream != NULL) {
		// La directory esiste
		closedir(directoryStream);
	} else if (errno == ENOENT) {
		// La directory non esiste
		returnCode = 0;
	} else {
		handle_error("Error checking if folder exists");
	}

	free(pathCopy);

	return returnCode;
}

#ifdef _WIN32
char *strsep(char **sp, char *sep) {
	char *p, *s;
	if  (sp == NULL || *sp == NULL || **sp == '\0') {
		return(NULL);
	}

	s = *sp;
	p = s + strcspn(s, sep);

	if (*p != '\0') {
		*p++ = '\0';
	}

	*sp = p;
	return(s);
}
#endif
