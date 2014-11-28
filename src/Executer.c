#include "Protocol.h"
#include "Executer.h"
#include "Utils.h"
#include "Wrapper.h"
#include "Config.h"
#include "Errors.h"
#include "dbg.h"
#include "Sync.h"
#include "Server/Server.h"

#include <string.h>
#include <dirent.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>

#ifndef _WIN32
	#include <sys/socket.h>
	#include <arpa/inet.h>
#else
	#include "windows.h"
	#include "WinBase.h"
	#include "winsock2.h"
#endif

#define PLACEHOLDER "placeholder"
int checkIdentifier(char* identifier, char* rootdir) {
	/* Cosa fare nel caso in cui l'identificatore non esiste sul server?
	In questa prima implementazione, si esce, avvisando l'utente. */

	int returnCode = ALLOK;

	if (strlen(identifier) != IDENTIFIER_LEN) {
		log_err("Bad identifier len: %s", identifier);
		return BAD_IDENTIFIER;
	}

	int i = 0;
	for (i = 0; i < strlen(identifier); ++i) {
		char currentChar = identifier[i];
		if (currentChar < 48 ||
		    (currentChar > 90 && currentChar < 97) ||
		    currentChar > 122) {
			return BAD_IDENTIFIER;
		}
	}

	char* identifierPath = createFullPath(rootdir, identifier);
	char* identifierFolderPath = createFullPath(identifierPath, PLACEHOLDER);

	if (checkFolderExists(identifierFolderPath) != 1) {
		log_err("Error opening identifier directory: %s", identifier);
		returnCode = BAD_IDENTIFIER;
	}

	free(identifierPath);
	free(identifierFolderPath);

	return returnCode;
}

char* createFullPath(char* firsts, char* seconds) {
	/* Unisce le tre stringhe passate in input, creando ad esempio:
		rootdir/id/path

	La stringa ritornata va liberata dal chiamante! */

	char* fullPath;
	if (firsts == NULL || seconds == NULL) {
		log_err("Trying to concatenate null strings.");
		fullPath = NULL;
	} else {
		fullPath = Malloc(sizeof(char) * (strlen(firsts) + 1 + strlen(seconds) + 1));
		sprintf(fullPath, "%s/%s", firsts, seconds);
	}

	return fullPath;
}

int executeServerCommand(int msfd, struct url *url, struct config *cfg) {
	/* In base al comando inserito, viene chiamata la rispettiva funzione. */

	// Si controlla l'esistenza dell'identificativo e si cambia directory.
	int returnCode = checkIdentifier(url->identifier, cfg->rootdir);

	if (returnCode != BAD_IDENTIFIER) {
		char* identifierPath = createFullPath(cfg->rootdir, url->identifier);
		char* command = url->command;

		if (strcmp(command, "mkdir") == 0) {
			returnCode = makeDir(identifierPath, url->argument);
		} else if (strcmp(command, "put") == 0) {
			returnCode = putf(msfd, identifierPath, url->argument, url->optionalArgument, "wb");
		} else if (strcmp(command, "puta") == 0) {
			returnCode = putf(msfd, identifierPath, url->argument, url->optionalArgument, "ab");
		} else if (strcmp(command, "move") == 0) {
			returnCode = movefd(identifierPath, url->argument, url->optionalArgument);
		} else if (strcmp(command, "dele") == 0) {
			returnCode = deletefd(identifierPath, url->argument);
		} else if (strcmp(command, "get") == 0) {
			returnCode = getf(msfd, identifierPath, url->argument);
		} else {
			log_err("Unknown_command");
			returnCode = MALFORMED_COMMAND;
		}

		free(identifierPath);
	} else {
		if (strcmp(url->command, "get") == 0) {
			/* Se l'identificativo non è valido, bisogna comunque
			avvertire il client che non sta per ricevere alcun file. */
			sendEntireFile(msfd, NULL);
		} else if (strcmp(url->command, "put") == 0 || strcmp(url->command, "puta") == 0) {
			/* E allo stesso modo devo avvertire il client, nel caso in cui
			lui voglia effettuare un put ma l'identificativo è errato. */
			sendReturnCode(msfd, returnCode);
		}
	}

	log_info("Return code: %d", returnCode);
	return returnCode;
}

int makeDir(char* identifierPath, char* path) {
	/* Questa funzione crea la directory, se il path esiste. */

	char* fullPath = createFullPath(identifierPath, path);
	int returnCode = ALLOK;

	SEMAPHORE_TYPE pathSemaphore = getPathSemaphore(fullPath, sharedMemory);

	#ifndef _WIN32
		int n;
		if ((n = sem_Trywait(pathSemaphore)) == EAGAIN) {
			returnCode = TEMP_UNAVAIBLE;
		} else {
			if (mkdir(fullPath, 0755) != 0) {
				log_err("Error while making new directory.");
				returnCode = parseErrno(errno);
			}

			sem_Post(pathSemaphore);
		}
		sem_Close(pathSemaphore);
	#else
		if (TRYWaitForSingleObject(pathSemaphore) == WAIT_TIMEOUT) {
			returnCode = TEMP_UNAVAIBLE;
		} else {
			if (mkdir(fullPath) != 0) {
				log_err("Error while making new directory.");
				returnCode = parseErrno(errno);
			}

			RELEASESemaphore(pathSemaphore);
		}
		CLOSEHandle(pathSemaphore);
	#endif

	free(fullPath);
	return returnCode;
}

int deletefd(char* identifierPath, char* path) {
	/* Questa funzione rimuove un file o una cartella (soltanto se vuota) */

	char* fullPath = createFullPath(identifierPath, path);
	int returnCode = ALLOK;

	SEMAPHORE_TYPE pathSemaphore = getPathSemaphore(fullPath, sharedMemory);

	#ifndef _WIN32
		if (sem_Trywait(pathSemaphore) == EAGAIN) {
			returnCode = TEMP_UNAVAIBLE;
		} else {
			if (remove(fullPath) != 0)	{
				log_err("Error while removing file or directory.");
				returnCode = parseErrno(errno);
			}

			sem_Post(pathSemaphore);
		}
		sem_Close(pathSemaphore);
	#else
		if (TRYWaitForSingleObject(pathSemaphore) == WAIT_TIMEOUT) {
			returnCode = TEMP_UNAVAIBLE;
		} else {
			DWORD ftype = GetFileAttributes(fullPath);

			if (ftype == INVALID_FILE_ATTRIBUTES) {  // path invalido
				returnCode = INVALID_PATH;
			} else if (ftype == FILE_ATTRIBUTE_DIRECTORY ) {  // directory
				if (_rmdir(fullPath) != 0)	{
					log_err("Error while removing file or directory.");
					returnCode = parseErrno(errno);
				}
			} else {
				if (remove(fullPath) != 0)	{
					log_err("Error while removing file or directory.");
					returnCode = parseErrno(errno);
				}
			}
			RELEASESemaphore(pathSemaphore);
		}

		CLOSEHandle(pathSemaphore);
	#endif

	free(fullPath);
	return returnCode;
}

int movefd(char* identifierPath, char* old, char* new) {
	/* Questa funzione sposta/rinomina un file o una cartella */
	int returnCode = ALLOK;

	char* oldFullPath = createFullPath(identifierPath, old);
	char* newFullPath = createFullPath(identifierPath, new);

	#ifndef _WIN32
		sem_t *moveSemaphore = sem_Open(SEMAPHORE_MOVE, 0);
		sem_Wait(moveSemaphore);
	#else
		HANDLE moveSemaphore = OPENSemaphore(SEMAPHORE_FLAGS, FALSE, SEMAPHORE_MOVE);
		WAITForSingleObjectInfinite(moveSemaphore);
	#endif

	SEMAPHORE_TYPE oldPathSemaphore = getPathSemaphore(oldFullPath, sharedMemory);
	SEMAPHORE_TYPE newPathSemaphore = getPathSemaphore(newFullPath, sharedMemory);

	#ifndef _WIN32
		if (sem_Trywait(oldPathSemaphore) == EAGAIN ||
		    sem_Trywait(newPathSemaphore) == EAGAIN)
		{
			sem_Post(moveSemaphore);
			returnCode = TEMP_UNAVAIBLE;
		} else {
			sem_Post(moveSemaphore);

			if (rename(oldFullPath, newFullPath) != 0) {
				log_err("Error while moving files");
				returnCode = parseErrno(errno);
			}

			sem_Post(newPathSemaphore);
			sem_Post(oldPathSemaphore);
		}

		sem_Close(oldPathSemaphore);
		sem_Close(newPathSemaphore);

	#else
		if (TRYWaitForSingleObject(oldPathSemaphore) == WAIT_TIMEOUT ||
		    TRYWaitForSingleObject(newPathSemaphore) == WAIT_TIMEOUT)
		{
			RELEASESemaphore(moveSemaphore);
			returnCode = TEMP_UNAVAIBLE;
		}
		else {
			RELEASESemaphore(moveSemaphore);

			if (rename(oldFullPath, newFullPath) != 0) {
				log_err("Error while moving files");
				returnCode = parseErrno(errno);
			}

			RELEASESemaphore(newPathSemaphore);
			RELEASESemaphore(oldPathSemaphore);
		}

		CLOSEHandle(newPathSemaphore);
		CLOSEHandle(oldPathSemaphore);
	#endif

	#ifndef _WIN32
		sem_Close(moveSemaphore);
	#else
		CLOSEHandle(moveSemaphore);
	#endif

	free(oldFullPath);
	free(newFullPath);
	return returnCode;
}

int putf(int msfd, char* identifierPath, char* argument, char* optionalArgument, char* mode) {
	int returnCode = ALLOK;

	char* path;
	if (optionalArgument == NULL || strcmp(optionalArgument, "") == 0) {
		path = argument;
	} else {
		path = optionalArgument;
	}

	char* fullPath = createFullPath(identifierPath, path);

	SEMAPHORE_TYPE pathSemaphore = getPathSemaphore(fullPath, sharedMemory);

	#ifndef _WIN32
		if (sem_Trywait(pathSemaphore) == EAGAIN) {
			returnCode = TEMP_UNAVAIBLE;
		} else {
			FILE* fstream;
			if ((fstream = fopen(fullPath, mode)) == NULL) {
			 	log_err("Error while opening file to save.");
			 	returnCode = parseErrno(errno);
			} else {
				fclose(fstream);

				// Segnalo al client che va tutto bene, può mandare il file
				sendReturnCode(msfd, returnCode);

				returnCode = recvSaveEntireFile(msfd, fullPath, mode);
				sem_Post(pathSemaphore);
			}
		}

		sem_Close(pathSemaphore);
	#else
		if (TRYWaitForSingleObject(pathSemaphore) == WAIT_TIMEOUT) {
			returnCode = TEMP_UNAVAIBLE;
		}
		else {
			FILE* fstream;
			if ((fstream = fopen(fullPath, mode)) == NULL) {
			 	log_err("Error while opening file to save.");
			 	returnCode = parseErrno(errno);
			} else {
				fclose(fstream);

				// Segnalo al client che va tutto bene, può mandare il file
				sendReturnCode(msfd, returnCode);
				returnCode = recvSaveEntireFile(msfd, fullPath, mode);

				RELEASESemaphore(pathSemaphore);
			}
		}

		CLOSEHandle(pathSemaphore);
	#endif

	free(fullPath);
	return returnCode;
}

int getf(int msfd, char* identifierPath, char* argument) {
	int returnCode = ALLOK;

	char* fullPath = createFullPath(identifierPath, argument);

	SEMAPHORE_TYPE pathSemaphore = getPathSemaphore(fullPath, sharedMemory);
	#ifndef _WIN32

		if (sem_Trywait(pathSemaphore) == EAGAIN) {
			returnCode = TEMP_UNAVAIBLE;
			sendEntireFile(msfd, NULL);
		} else {
			returnCode = sendEntireFile(msfd, fullPath);
			sem_Post(pathSemaphore);
		}

		sem_Close(pathSemaphore);
	#else
		if (TRYWaitForSingleObject(pathSemaphore) == WAIT_TIMEOUT) {
			returnCode = TEMP_UNAVAIBLE;
			sendEntireFile(msfd, NULL);
		} else {
			returnCode = sendEntireFile(msfd, fullPath);
			RELEASESemaphore(pathSemaphore);
		}

		CLOSEHandle(pathSemaphore);
	#endif

	free(fullPath);
	return returnCode;
}

int sendReturnCode(int msfd, uint32_t returnCode) {
	returnCode = htonl(returnCode);

	int n;
	if ((n = send(msfd, CHARCAST &returnCode, sizeof(uint32_t), 0)) < 1) {
		log_err("Error while sending return code to client.");
	}

	return n;
}
