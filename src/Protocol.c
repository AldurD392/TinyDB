#include "Protocol.h"
#include "Wrapper.h"
#include "Utils.h"
#include "Config.h"
#include "Errors.h"
#include "dbg.h"
#include "Executer.h"
#include "Logger.h"
#include "Sync.h"
#include "Server/Server.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#ifndef _WIN32
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
#else
	#include <winsock2.h>
	#include <Ws2tcpip.h>

	#ifndef MSG_WAITALL
		#define MSG_WAITALL 0x8 /* do not complete until packet is completely filled */
	#endif
#endif

int parseArgv(char* argv, struct url *url) {
	/* Questa funzione prende come argomenti:
	 -	una stringa di URL nel formato: mydb://xxx.yyy.www.zzz/<identifier>/<command>/<argument>{<optionalArgument>}
	 -	un puntatore ad una struct di tipo url che compilerà con gli appositi campi.
	e compila gli opportuni campi, con gli argomenti passati nella stringa.
	Attenzione, questa funzione modifica la stringa originale, sta al chiamante duplicarla nel caso voglia mantenerla.
	 */

	int returnCode = ALLOK;

	// Controllo il protocollo:
	char* protocol = Calloc(1, sizeof(char) * (STARTING + 1));
	if (strncpy(protocol, argv, STARTING) == NULL) {
		handle_error("Error copying protocol string");
	}

	if (strcmp(protocol, "mydb://") != 0) {
		returnCode = MALFORMED_COMMAND;
	} else {
		argv += STARTING;

		// Riempio i campi
		url->address = strsep(&argv, "/");
		url->identifier = strsep(&argv, "/");
		url->command = strsep(&argv, "/");
		url->argument = strsep(&argv, "{");
		url->optionalArgument = strsep(&argv, "}");
	}

	free(protocol);
	return returnCode;
}

void* readAll(int sfd, size_t type, size_t* bufferSize)
{
	/* Questa funzione legge da un filedescriptor, nell'ordine:
	-	il numero di byte da leggere;
	-	contenuto per il numero di byte indicato in precedenza.
	In caso di impossibilità di invio,
	segnalata dall'altra parte del socket con una dimensione pari a 0,
	viene ritornato NULL e dimensione del buffer pari a 0.
	In caso di errore di read dal socket, viene ritornato NULL ma dimensione del buffer diversa da 0.
	*/

	uint64_t size = 0;
	int n = 0;
	if ((n = recv(sfd, CHARCAST &size, sizeof(uint64_t), MSG_WAITALL)) < 1) {
		log_err("Error on receiving file size.");
		return NULL;
	}

	size = ntohl(size);
	if (size == 0) {
		if (bufferSize != NULL)	{
			*bufferSize = size;
		}
		return NULL;
	}

	if (bufferSize != NULL)	{
		size_t sizeBackup = (size_t)size;
		*bufferSize = sizeBackup;
	}

	void* buffer = Calloc(1, type * size);
	void* savedBuffer = buffer;

	while (size > 0) {
		if ((n = recv(sfd, savedBuffer, type * size, 0)) < 1) {
			log_err("Error on receiving file content.");
			return NULL;
		}
		savedBuffer += n/type;
		size -= n;
	}

	return buffer;
}

int sendEntireFile(int sfd, char* path)
{
	/* Questa funzione, invia l'intero contenuto del file specificato da path,
	attraverso il socket sfd, inviando prima la dimensione,
	e in seguito il contenuto del file.
	Se il flag pathCoherence è settato a 0, allora non è possibile inviare file
	utilizzando un path assoluto. */

	uint64_t size = 0;
	int n = 0;

	FILE* fstream;
	if (path == NULL || (fstream = fopen(path, "rb")) == NULL) {
		/* Per indicare dall'altra parte, l'impossibilità di completare la richiesta,
		si invia uno zero, che verrà interpretato come dimensione 0, e di conseguenza,
		dimensione non valida per un file da ricevere.

		Tali situazioni, possono capitare se si cerca di inviare un file invalido, o se,
		il path del file da inviare è NULL. In tal caso infatti, questa funzione, viene utilizata,
		soltanto per avvertire dall'altra parte del socket dell'impossibilità di completare
		la richiesta.
		*/

		log_err("Error while fopening file to send or file locked.");

		size = 0; // Non serve specificare l'endian
		if ((n = send(sfd, CHARCAST &size, sizeof(uint64_t), 0)) == -1) {
			log_err("Error while sending 0 as file size, \
			        to tell there was a problem with the given file.");
			return parseErrno(errno); // potrebbe essere settato manualmente a ?
		};

		return parseErrno(errno); // potrebbe essere settato manualmente a INVALID_PATH
	}

	fseek(fstream, 0, SEEK_END);
	size = ftell(fstream);
	size = htonl(size); // Dimensione del file in network endian

	if ((n = send(sfd, CHARCAST &size, sizeof(uint64_t), 0)) == -1) {
		log_err("Error while sending file size.");
		return parseErrno(errno); // potrebbe essere settato manualmente a ?
	};

	fseek(fstream, 0, SEEK_SET); // Riparto dall'inizio del file

	void* buffer = Calloc(1, BIG_BUFFER_SIZE);
	while (!feof(fstream)) {
		int n = fread(buffer, 1, BIG_BUFFER_SIZE, fstream);
		if (ferror(fstream)) {
		   log_err("Error while reading file content.");
		   return parseErrno(errno); // potrebbe essere settato manualmente a ?
		}
		int wn = send(sfd, buffer, n, 0);
		if (wn < n) {
			log_err("Error while sending file content.");
			return parseErrno(errno); // potrebbe essere settato manualmente a ?
		}
	}

	free(buffer);
	fclose(fstream);

	return ALLOK;
}

int recvSaveEntireFile(int sfd, char* path, char* mode)
{
	size_t bufferSize;
	void* buffer = readAll(sfd, 1, &bufferSize);

	if (buffer == NULL && bufferSize != 0)	{
		log_err("Error while receiving file.");
		return parseErrno(errno); // potrebbe essere settato manualmente a CANNOT_SAVE_FILE
	} else if (buffer == NULL && bufferSize == 0) {
		log_err("The other side couldn't send requested content.");
		return INVALID_PATH; // si ritorna come codice di errore il path invalido.
	}

	FILE* fstream;
	if ((fstream = fopen(path, mode)) == NULL) {
	 	log_err("Error while opening file to save.");
	 	return parseErrno(errno);
	}

	int n;
	if ((n = fwrite(buffer, 1, bufferSize, fstream)) < bufferSize) {
		log_err("Error while saving file.");
		return parseErrno(errno); // potrebbe essere settato manualmente a CANNOT_SAVE_FILE
	};

	fclose(fstream);
	free(buffer);

	return ALLOK;
}

void acceptOnSocket(struct acceptArgs *args) {

	#ifdef _WIN32
	HANDLE threadHandle = CreateEvent(NULL,
	                               TRUE,
	                               FALSE,
	                               THREAD_HANLDER_NAME);
	#endif

	while (1) {

		socklen_t size = sizeof(struct sockaddr_storage);
		struct sockaddr_storage *managedSocketAddress = Calloc(1, size);
		SOCKET_TYPE msfd;

		if ((msfd = accept(args->sfd, (struct sockaddr *) managedSocketAddress, &size)) == -1) {
			#ifdef _WIN32
				// Qui si controlla se l'accept è fallita perché il padre ha chiuso la connessione
				if ((WSAGetLastError() != WSAEINTR &&
				    WSAGetLastError() != WSAENOTSOCK) ||
				    WaitForSingleObject(threadHandle, 0) != WAIT_OBJECT_0)
				{
					handle_error("Error accepting connection on socket");
				} else {
					CLOSEHandle(threadHandle);
					free(managedSocketAddress);
					ExitThread(1);
				}
			#else
				handle_error("Error accepting connection on socket");
			#endif
		}

		struct url* command = Calloc(1, sizeof(struct url));

		char* buffer = readAll(msfd, sizeof(char), NULL);
		if (buffer == NULL) {
			handle_error("Error while receiving command.");
		}

		parseArgv(buffer, command);

		uint32_t returnCode = executeServerCommand(msfd, command, args->cfg);

		sendReturnCode(msfd, returnCode);

	#ifndef _WIN32
		sem_t *logSemaphore = sem_open(SEMAPHORE_LOG, 0);
		sem_Wait(logSemaphore);
	#else
		HANDLE logSemaphore = OPENSemaphore(SEMAPHORE_FLAGS, FALSE, SEMAPHORE_LOG);
		WAITForSingleObjectInfinite(logSemaphore);
	#endif

		logAppend(args->cfg->logpath, returnCode, managedSocketAddress, size, command);

	#ifndef _WIN32
		sem_Post(logSemaphore);
	#else
		RELEASESemaphore(logSemaphore);
		CLOSEHandle(logSemaphore);
	#endif

		close(msfd);
		free(buffer);
		free(command);
		free(managedSocketAddress);
	}

}
