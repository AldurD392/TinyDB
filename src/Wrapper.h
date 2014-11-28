#ifndef __Wrapper_h__
#define __Wrapper_h__

#include <stdlib.h>

#ifndef _WIN32
	#include <pthread.h>
	#include <semaphore.h>
#else
	#include <Windows.h>
	#include <winbase.h>
#endif

#ifndef _WIN32
	void sem_Post(sem_t *sem);
	void sem_Wait(sem_t *sem);
	sem_t *sem_Open(const char *name, int oflag);
	sem_t *sem_OpenCreate(const char *name, int oflag, mode_t mode, unsigned int value);
	void sem_Close(sem_t *sem);
	void sem_Unlink(const char *name);
	int sem_Trywait(sem_t *sem);
#else
	HANDLE CREATESemaphore(LPSECURITY_ATTRIBUTES lsSemaphoreAttributes,
	                       LONG lInitialCount,
	                       LONG lMaximumCount,
	                       LPCTSTR lpName);

	HANDLE WINAPI OPENSemaphore(
		DWORD dwDesiredAccess,
		BOOL bInheritHandle,
		LPCTSTR lpName
	);

	void WAITForSingleObjectInfinite(HANDLE hHandle);
	DWORD TRYWaitForSingleObject(HANDLE hHandle);
	void CLOSEHandle(HANDLE hObject);
	void RELEASESemaphore(HANDLE hSemaphore);
#endif

void *Malloc(size_t size);
void *Calloc(size_t nmemb, size_t size);
void *Realloc(void *ptr, size_t size);
void *Alloca(size_t size);

#endif
