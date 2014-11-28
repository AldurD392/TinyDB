#ifndef __Server_h__
#define __Server_h__

#ifndef _WIN32
	#include <pthread.h>
	#include <semaphore.h>
#else
	#define CTRL_HANDLER_NAME TEXT("ctrlHandler")
	#define THREAD_HANLDER_NAME TEXT("threadHandler")
	#include <Windows.h>
#endif

void* sharedMemory;

#ifndef _WIN32
	void* createAttachSharedMemory(int* shmId, int flags);
	void cleanExit(int signo);
#else
	BOOL ctrlHandler(DWORD fwdCtrlType);
#endif

#endif
