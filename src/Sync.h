#ifndef __Sync_h__
#define __Sync_h__

#ifndef _WIN32
	#include <pthread.h>
	#include <semaphore.h>
#else
	#include <windows.h>
#endif

#include <stdint.h>

#define SEMAPHORE_LOG "/logsemaphore"
#define SEMAPHORE_MOVE "/movesemaphore"
#define SEMAPHORE_SHM "/shmsemaphore"

#ifndef _WIN32
	#define SEMAPHORE_FLAGS 0644
#else
	#define SEMAPHORE_FLAGS SEMAPHORE_MODIFY_STATE | SYNCHRONIZE
#endif

#define SHM_PATH "shm_path"
#define SHM_KEY 0000070
#define SHM_SIZE 1024 * sizeof(uint32_t)
#define SHM_FLAGS 0644 | IPC_CREAT

#ifndef _WIN32
	#define SEMAPHORE_TYPE sem_t*
#else
	#define SEMAPHORE_TYPE HANDLE
#endif

void addNewSemaphoreName(uint32_t hashedPath, void* sharedMemory);
void removePathSemaphores(void* sharedMemory);
SEMAPHORE_TYPE getPathSemaphore(char* path, void* sharedMemory);

#endif
