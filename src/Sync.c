#include "Sync.h"
#include "Utils.h"
#include "Wrapper.h"
#include "dbg.h"
#include "crc32.h"

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>

#define BUFFER_SIZE 12

#ifndef _WIN32
	void removePathSemaphores(void* sharedMemory) {
		/* Presi i nomi dei semafori in memoria virtuale,
		ad uno ad uno ne prende i nomi e li distrugge. */

		sem_t *sharedMemorySemaphore = sem_Open(SEMAPHORE_SHM, 0);
		sem_Wait(sharedMemorySemaphore);

		uint32_t last;
		if (memcpy(&last, sharedMemory, sizeof(uint32_t)) == NULL) {
			handle_error("Error reading shm size");
		}

		char* hashedPathString = Malloc(sizeof(char) * BUFFER_SIZE);
		uint32_t* castedMemory = (uint32_t*) sharedMemory;
		int i;
		for (i = 1; i <= (last / sizeof(uint32_t)); ++i) {
			if (snprintf(hashedPathString, BUFFER_SIZE, "%u", castedMemory[i]) < 0) {
				handle_error("Error converting hash to string");
			}
			sem_Unlink(hashedPathString);
		}

		free(hashedPathString);
		sem_Post(sharedMemorySemaphore);
		sem_Close(sharedMemorySemaphore);
		return;
	}

	sem_t* getPathSemaphore(char* path, void* sharedMemory) {
		sem_t *pathSemaphore = NULL;

		uint32_t hashedPath = crc32(CRC_HASHING_CONSTANT, path, strlen(path) + 1);

		char* hashedPathString = Malloc(sizeof(char) * BUFFER_SIZE);
		if (snprintf(hashedPathString, BUFFER_SIZE, "%u", hashedPath) < 0) {
			handle_error("Error converting hash to string");
		}


		if ((pathSemaphore = sem_open(hashedPathString, 0)) == SEM_FAILED) {
			if (errno != ENOENT) {
				handle_error("Error while opening path semaphore");
			} else {
				pathSemaphore = sem_OpenCreate(hashedPathString, O_CREAT, SEMAPHORE_FLAGS, 1);
				addNewSemaphoreName(hashedPath, sharedMemory);
			}
		}

		free(hashedPathString);
		return pathSemaphore;
	}

	void addNewSemaphoreName(uint32_t hashedString, void* sharedMemory) {
		/*
			Ottiene un lock sulla memoria condivisa.
			Dal buffer di memoria condivisa, innanzitutto legge l'offset dell'ultimo elemento allocato.
			Se non vi sono stringhe, o la stringa non esiste, viene inserita, ed aggiornato l'offset.
			Rilascia il lock.
		*/

		sem_t *sharedMemorySemaphore = sem_Open(SEMAPHORE_SHM, 0);
		sem_Wait(sharedMemorySemaphore);

		uint32_t last;
		if (memcpy(&last, sharedMemory, sizeof(uint32_t)) == NULL) {
			handle_error("Error reading shm size");
		}

		if (memcpy(sharedMemory + last + sizeof(uint32_t), &hashedString, sizeof(uint32_t)) == NULL) {
			handle_error("Error writing string to shm");
		}

		last += sizeof(uint32_t);
		if (memcpy(sharedMemory, &last, sizeof(uint32_t)) ==  NULL){
			handle_error("Error writing new size to shm");
		}

		sem_Post(sharedMemorySemaphore);
		sem_Close(sharedMemorySemaphore);

		return;
	}

#else
	HANDLE getPathSemaphore(char* path, void* sharedMemory) {
		HANDLE pathSemaphore = NULL;

		uint32_t hashedPath = crc32(CRC_HASHING_CONSTANT, path, strlen(path) + 1);

		char* hashedPathString = Malloc(sizeof(char) * BUFFER_SIZE);
		if (snprintf(hashedPathString, BUFFER_SIZE, "%u", hashedPath) < 0) {
			handle_error("Error converting hash to string");
		}

		if ((pathSemaphore = OpenSemaphore(SEMAPHORE_FLAGS, FALSE, hashedPathString)) == NULL) {
			if (GetLastError() != ERROR_FILE_NOT_FOUND) {
				handle_error("Error while opening path semaphore");
			} else {
				pathSemaphore = CREATESemaphore(NULL, 1, 1, hashedPathString);
			}
		}

		free(hashedPathString);
		return pathSemaphore;
	}
#endif
