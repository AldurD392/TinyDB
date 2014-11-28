#include "../Utils.h"
#include "../Protocol.h"
#include "../dbg.h"
#include "../Wrapper.h"
#include "../Config.h"
#include "../Errors.h"

#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>

#ifndef _WIN32
	#define CHARCAST
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
#else
	#ifndef MSG_WAITALL
		#define MSG_WAITALL 0x8
	#endif

	#define CHARCAST (char *)
	#include <winsock2.h>
	#include <Ws2tcpip.h>
	#include <io.h>
#endif

int readReturnCode(int sfd) {
	// Il client legge il valore di ritorno
	uint32_t returnCode;

	if ((recv(sfd, CHARCAST &returnCode, sizeof(uint32_t), MSG_WAITALL)) < 1) {
		handle_error("Error while getting return code");
	}

	returnCode = ntohl(returnCode);
	return returnCode;
}

int checkCommand(struct url* url) {
	// Controllo (almeno finché possibile) dei parametri passati quando si tratta di get e put.
	if (url->argument == NULL || strcmp(url->argument, "") == 0) {
		log_err("The argument is mandatory");
		return MALFORMED_COMMAND;
	}

	else if (strcmp(url->command, "put") == 0 || strcmp(url->command, "puta") == 0) {
		if (checkFileExists(url->argument) == 0) {
			log_err("The file you want to put doesn't exist.");
			return INVALID_PATH;
		}
	}

	else if (strcmp(url->command, "get") == 0) {
		int returnCode = 1;
		if (url->optionalArgument == NULL || strcmp(url->optionalArgument, "") == 0) {
			returnCode = checkFolderExists(url->argument);
		} else {
			returnCode = checkFolderExists(url->optionalArgument);
		}
		if (returnCode == 0) {
			log_err("Invalid path");
			return INVALID_PATH;
		}
	}

	else if (strcmp(url->command, "move") == 0) {
		if (url->optionalArgument == NULL || strcmp(url->optionalArgument, "") == 0) {
			log_err("The optional argument is mandatory");
			return MALFORMED_COMMAND;
		}
		else if (strcmp(url->argument, url->optionalArgument) == 0) { /* to avoid move(x,x) command */
			log_err("Argument and optional argument are the same");
			return MALFORMED_COMMAND;
		}
	}

	return ALLOK;
}

int executeCommand(int sfd, struct url* url)
{
	int executeCode = ALLOK;

	if (strcmp(url->command, "put") == 0 || strcmp(url->command, "puta") == 0) {
		if ((executeCode = readReturnCode(sfd)) != ALLOK) {
			return executeCode;
		} else {
			executeCode = sendEntireFile(sfd, url->argument);
		}
	}

	else if (strcmp(url->command, "get") == 0) {
		char* path;

		if (url->optionalArgument == NULL || strcmp(url->optionalArgument, "") == 0) {
			path = url->argument;
		} else {
			path = url->optionalArgument;
		}

		executeCode = recvSaveEntireFile(sfd, path, "wb");
	}

	return readReturnCode(sfd);
}

int printInfo(const char* exePath) {
	printf("Inserire almeno un comando, nel formato:\
	       \n%s [-P <port>] mydb://<address>/<identifier>/<command>/<arg>{<optarg>} ...\n", exePath);
	return MALFORMED_COMMAND;
}

int main(int argc, char const *argv[])
{
	const char* port = PORT;
	int i = 1;
	if (argc < 2) {
		return printInfo(argv[0]);
	} else {
		if (strcmp(argv[1], "-P") == 0) {
			if (argc < 3) {
				printf("Numero di porta non valido. \n");
				return MALFORMED_COMMAND;
			} else if (argc < 4) {
				return printInfo(argv[0]);
			} else {
				port = argv[2];
				i = 3;
			}
		}
	}

	while (i < argc) {
		struct url *url;
		url = Calloc(1, sizeof(struct url));

		char* command = strdup(argv[i]);
		int checkCode = parseArgv(command, url);

		if (checkCode != ALLOK || (checkCode = checkCommand(url)) != ALLOK)	{
			printf("Impossibile eseguire il comando \"%s\" passato in input. Path o comando non valido?\n", argv[i]);
			return checkCode;
		}

		// 1-3 Creazione e inizializzazione del socket
		SOCKET_TYPE sfd = createClientSocket(url->address, port);

		// 4. Lettura/scrittura sul socket
		int n;
		uint64_t size = htonl(sizeof(char) * strlen(argv[i]) + 1);
		if ((n = send(sfd, CHARCAST &size, sizeof(uint64_t), 0)) == -1) { // Scrivo la dimensione della riga di input.
			handle_error("Error while sending command size");
		}
		if ((n = send(sfd, argv[i], (sizeof(char) * strlen(argv[i]) + 1), 0)) == -1) { // Scrivo tutta la stringa di input.
			handle_error("Error while sending command");
		}

		printf("Il codice di ritorno è: %d\n", executeCommand(sfd, url));

		// 5. Cleanup!
		close(sfd);
		free(url);
		free(command);

		i++;
	}

	return 0;
}
