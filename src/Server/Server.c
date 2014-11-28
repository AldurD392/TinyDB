#include "Server.h"
#include "threadedServer.h"
#include "forkedServer.h"

#include "../Utils.h"
#include "../Protocol.h"
#include "../Wrapper.h"
#include "../dbg.h"
#include "../Config.h"
#include "../Executer.h"
#include "../Errors.h"
#include "../Logger.h"
#include "../Sync.h"

#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>

#ifndef _WIN32
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <sys/wait.h>
	#include <semaphore.h>
	#include <sys/ipc.h>
	#include <sys/shm.h>
	#include <signal.h>

	#define SLEEP(n) sleep(n)
#else
	// #include <Windows.h>
	#if NTDDI_VERSION > NTDDI_WIN7
		#include <Synchapi.h>
	#else
		#include <winbase.h>
		#include <WinBase.h>
	#endif

	#define SLEEP(n) Sleep(n * 1000)
#endif

void initSharedMemory(void* sharedMemory) {
	uint32_t n = 0;
	if (memcpy(sharedMemory, &n, sizeof(uint32_t)) == NULL) {
		handle_error("Error while initialiting shm");
	}
}

#ifndef _WIN32
	void* createAttachSharedMemory(int* shmId, int flags) {
		if ((*shmId = shmget(SHM_KEY, SHM_SIZE, flags)) == -1) {
			handle_error("Error getting shared memory");
		}

		void* sharedMemory = shmat(*shmId, (void *)0, 0);
		if (sharedMemory == (char *)(-1)) {
		    handle_error("Error while attaching to shared memory");
		}

		initSharedMemory(sharedMemory);

		return sharedMemory;
	}

	void destroySharedMemory(int shmId) {
		if (shmctl(shmId, IPC_RMID, NULL) == -1) {
			handle_error("Error while marking shm for destroy");
		}
	}
#endif

#ifdef _WIN32
	BOOL ctrlHandler(DWORD fwdCtrlType) {
		if (fwdCtrlType == CTRL_C_EVENT) {
			HANDLE ctrlHandle = CreateEvent(NULL,
			                               TRUE,
			                               FALSE,
			                               CTRL_HANDLER_NAME);
			if (ctrlHandle == NULL) {
				handle_error("Error while creating event");
			}

			if (SetEvent(ctrlHandle) == 0) {
				handle_error("Error while setting event");
			}

			return TRUE;
		} else {
			return FALSE;
		}
	}
#endif

#ifndef _WIN32
	void daemonize() {
		pid_t pid = -1;
		pid = fork();

		if (pid == 0) {
			pid_t pgid = setsid();
			if (pgid == -1) {
				handle_error("Error setting sid");
			}

			if (signal(SIGHUP, NULL) == SIG_ERR) {
				handle_error("Error on signal");
			}

			pid_t npid = -1;
			npid = fork();

			if (npid == 0) {
				return;
			} else if (npid > 0) {
				exit(0);
			} else {
				handle_error("Error on fork");
			}
		} else if (pid > 0) {
			exit(0);
		} else {
			handle_error("Error on fork");
		}
	}
#endif

#ifndef _WIN32
	sem_t *logSemaphore, *moveSemaphore, *shmSemaphore;
	int shmId = 0;

	void destroyUsedResources() {
		removePathSemaphores(sharedMemory);  // Distruggo i semafori creati

		if (shmdt(sharedMemory) != 0) {
			handle_error("Error while detaching shared memory");
		}

		destroySharedMemory(shmId);

		sem_Close(moveSemaphore);
		sem_Unlink(SEMAPHORE_MOVE);

		sem_Close(logSemaphore);
		sem_Unlink(SEMAPHORE_LOG);

		sem_Close(shmSemaphore);
		sem_Unlink(SEMAPHORE_SHM);
	}

	void cleanExit(int signo) {
		destroyUsedResources();  // Distruggo i semafori creati

		exit(0);
	}
#endif


int main(int argc, char const *argv[]) {
	char* port = NULL;
	if (argc == 3) {
		if (strcmp(argv[1], "-P") == 0) {
			log_info("Specified command line port: ignoring conf");
			port = strdup(argv[2]);
		} else {
			log_info("Invalid command line parameters, ignoring.");
		}
	} else if (argc != 1) {
		log_info("Invalid command line parameters, ignoring.");
	}

	#ifdef DAEMON
		#ifndef _WIN32
			daemonize();
		#endif
	#endif

	while(1) {
		struct config *cfg = loadConfiguration();

		if (port != NULL) {
			free(cfg->port);
			cfg->port = strdup(port);
		}

		#ifndef _WIN32
			logSemaphore = sem_OpenCreate(SEMAPHORE_LOG, O_CREAT, SEMAPHORE_FLAGS, 1);  // Semaforo del log
			moveSemaphore = sem_OpenCreate(SEMAPHORE_MOVE, O_CREAT, SEMAPHORE_FLAGS, 1);  // Semaforo per il move
			shmSemaphore = sem_OpenCreate(SEMAPHORE_SHM, O_CREAT, SEMAPHORE_FLAGS, 1);  // Semaforo per la memoria condivisa

			shmId = 0;
			sharedMemory = createAttachSharedMemory(&shmId, SHM_FLAGS);  // Globale
		#else
			HANDLE logSemaphore = CREATESemaphore(NULL, 1, 1, SEMAPHORE_LOG);  // Semaforo del log
			HANDLE moveSemaphore = CREATESemaphore(NULL, 1, 1, SEMAPHORE_MOVE);  // Semaforo per il move
			HANDLE shmSemaphore = CREATESemaphore(NULL, 1, 1, SEMAPHORE_SHM);  /* Semaforo per impedire che due processi possano
																					eseguire la open contemporaneamente */
		#endif

		int len;
		SOCKET_TYPE* sfds = createServerSocket(cfg->port, &len);

		log_info("Il server è pronto.");
		if (cfg->process == 1) {
			#ifndef _WIN32
				serverMultiProcesses(sfds, len, cfg);
			#else
				char* argvPath = NULL;
				if ((argvPath = strdup(argv[0])) == NULL) {
					handle_error("Error duplicating argv");
				}
				serverMultiProcesses(sfds, len, cfg, argvPath);
				free(argvPath);
			#endif
		} else {
			serverMultiThreads(sfds, len, cfg);
		}

		int i;
		for (i = 0; i < len; ++i) {
			#ifndef _WIN32
				if (close(sfds[i]) != 0) {
					log_err("Error on closing sockets");
				}
			#else
				if (closesocket(sfds[i]) == SOCKET_ERROR && WSAGetLastError() != WSAENOTSOCK) {
					log_err("Error while closing sockets");
				}
			#endif
		}
		#ifdef _WIN32
			if (WSACleanup() != 0) {
				log_err("Error cleaning WSA");
			}
		#endif
		free(sfds);

		// Libero la struct di configurazione
		freeConfig(cfg);

		#ifndef _WIN32
			destroyUsedResources();
		#else
			CLOSEHandle(moveSemaphore);
			CLOSEHandle(logSemaphore);
			CLOSEHandle(shmSemaphore);
		#endif

		SLEEP(1); // Solo per debug, permette di terminare "comodamente" il server
	}

	free(port);
	return 0;
}
