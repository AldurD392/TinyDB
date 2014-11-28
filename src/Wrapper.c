#include "Wrapper.h"
#include "Utils.h"
#include "dbg.h"

#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

#ifndef _WIN32
	#include <pthread.h>
	#include <semaphore.h>
#else
	#include <Windows.h>
	#include <winbase.h>
	#include <malloc.h>
#endif

/* Semplici wrapper per il controllo degli errori. */

void *Malloc(size_t size) {
	if (size == 0) {
		handle_error("Cannot malloc a size of 0!");
	}
	void* ptr = malloc(size);
	if (ptr == NULL) {
		handle_error("Error on malloc");
	}
	return ptr;
}

void *Calloc(size_t nmemb, size_t size) {
	if (size == 0) {
		handle_error("Cannot Calloc a size of 0!");
	}
	void* ptr = calloc(nmemb, size);
	if (ptr == NULL) {
		handle_error("Error on calloc");
	}
	return ptr;
}

void *Realloc(void *ptr, size_t size) {
	if (size == 0) {
		handle_error("Cannot Realloc a size of 0!");
	}
	ptr = realloc(ptr, size);
	if (ptr == NULL) {
		handle_error("Error on realloc");
	}
	return ptr;
}

void *Alloca(size_t size) {
	if (size == 0) {
		handle_error("Cannot Alloc a size of 0!");
	}

	void* ptr = alloca(size);
	if (ptr == NULL) {
		handle_error("Error while allocaing");
	}

	return ptr;
}

#ifndef _WIN32
	void sem_Post(sem_t *sem) {
		if (sem_post(sem) != 0) {
			handle_error("Error posting on semaphore");
		}
	}

	void sem_Wait(sem_t *sem) {
		if (sem_wait(sem) != 0) {
			handle_error("Error waiting on semaphore");
		}
	}

	sem_t *sem_Open(const char *name, int oflag) {
		sem_t *semaphore = NULL;

		if ((semaphore = sem_open(name, oflag)) == SEM_FAILED) {
			handle_error("Error while opening semaphore");
		}

		return semaphore;
	}

	sem_t *sem_OpenCreate(const char *name, int oflag, mode_t mode, unsigned int value) {
		sem_t *semaphore = NULL;

		if ((semaphore = sem_open(name, oflag, mode, value)) == SEM_FAILED) {
			handle_error("Error while opening semaphore");
		}

		return semaphore;
	}

	void sem_Close(sem_t *sem) {
		if (sem_close(sem) != 0) {
			handle_error("Error closing semaphore");
		}
	}

	void sem_Unlink(const char *name) {
		if (sem_unlink(name) != 0) {
			handle_error("Error unlining semaphore");
		}
	}

	int sem_Trywait(sem_t *sem) {
		int returnCode = 0;
		if ((returnCode = sem_trywait(sem)) != 0 && errno == EAGAIN) {
			returnCode = EAGAIN;
		} else if (returnCode != 0) {
			handle_error("Error while trywaiting semaphore");
		}

		return returnCode;
	}
#else
	HANDLE CREATESemaphore(LPSECURITY_ATTRIBUTES lsSemaphoreAttributes,
	                       LONG lInitialCount,
	                       LONG lMaximumCount,
	                       LPCTSTR lpName) {
		HANDLE returnHandle = CreateSemaphore(lsSemaphoreAttributes, lInitialCount, lMaximumCount, lpName);

		if (returnHandle == NULL) {
			handle_error("Error while creating semaphore");
		}

		return returnHandle;
	}

	HANDLE WINAPI OPENSemaphore(DWORD dwDesiredAccess,
								BOOL bInheritHandle,
								LPCTSTR lpName
	) {
		HANDLE returnHandle = OpenSemaphore(dwDesiredAccess, bInheritHandle, lpName);

		if (returnHandle == NULL) {
			handle_error("Error while opening semaphore");
		}

		return returnHandle;
	}

	void WAITForSingleObjectInfinite(HANDLE hHandle) {
		if (WaitForSingleObject(hHandle, INFINITE) == WAIT_FAILED) {
			handle_error("Error waiting for single object");
		}

		return;
	}

	DWORD TRYWaitForSingleObject(HANDLE hHandle) {
		DWORD returnCode = -1;
		if ((returnCode = WaitForSingleObject(hHandle, 0)) == WAIT_FAILED) {
			handle_error("Error waiting for single object");
		}

		return returnCode;
	}

	void CLOSEHandle(HANDLE hObject) {
		if (CloseHandle(hObject) == 0) {
			handle_error("Error while closing handle");
		}

		return;
	}

	void RELEASESemaphore(HANDLE hSemaphore) {
		if (ReleaseSemaphore(hSemaphore, 1, NULL) == 0) {
			handle_error("Error while releasing semaphore");
		}

		return;
	}
#endif
