#ifndef __forkedServer_h__
#define __forkedServer_h__

#include "../Config.h"
#include "../Utils.h"
#include "../Protocol.h"

#include <unistd.h>

#ifdef _WIN32
	#define PROCESSED_SERVER_EXE "ProcessedServer.exe"
#endif

#ifndef _WIN32
	void killForkedProcesses(pid_t* pids, int len);
	void serverMultiProcesses(SOCKET_TYPE* sfds, int sfdslen, struct config* cfg);
#else
	void killCreatedProcesses(HANDLE* processHandles, int len);
	void serverMultiProcesses(SOCKET_TYPE* sfds, int sfdslen, struct config* cfg, char* argvPath);
#endif

#endif
