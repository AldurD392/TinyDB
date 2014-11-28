#include "forkedServer.h"
#include "../Config.h"
#include "../Wrapper.h"
#include "../Utils.h"
#include "../dbg.h"
#include "../Sync.h"
#include "../Executer.h"
#include "Server.h"

#include <unistd.h>
#include <signal.h>
#include <stdlib.h>

#ifndef _WIN32
	#include <sys/wait.h>
#else

#endif

#ifndef _WIN32
	void killForkedProcesses(pid_t* pids, int len) {
		int i;
		for (i = 0; i < len; ++i) {
			if (kill(pids[i], SIGTERM) < 0) {
				log_err("Error while killing process");
			};
			if (waitpid(pids[i], NULL, 0) < 0) {
				log_err("Error on waitpid");
			}
		}
	}
#else
	void killCreatedProcesses(HANDLE* processHandles, int len) {
		int i;
		if (GenerateConsoleCtrlEvent(CTRL_C_EVENT, 0) == 0) {
			handle_error("Error while generating console console event");
		}
		for (i = 0; i < len; ++i) {
			if (WaitForSingleObject(processHandles[i], INFINITE) == WAIT_FAILED) {
				log_err("Error on waitpid");
			}
			CLOSEHandle(processHandles[i]);
		}
	}
#endif


#ifndef _WIN32
	void serverMultiProcesses(int* sfds, int sfdslen, struct config* cfg) {
		pid_t *pids = Malloc(sizeof(pid_t) * cfg->nconcurrent);
		pid_t pid = -1;

		int sfd;
		struct acceptArgs* args = Calloc(1, sizeof(struct acceptArgs));
		args->cfg = cfg;

		int i;
		for (i = 0; i < cfg->nconcurrent; ++i) {
			sfd = sfds[i % sfdslen];
			pid = fork();

			if (pid < 0) {
				handle_error("Error on fork");
			} else if (pid == 0) {
				char* shm_path = createFullPath(cfg->rootdir, SHM_PATH);
				int shmId = 0;
				sharedMemory = createAttachSharedMemory(&shmId, 0);  // Globale!
				free(shm_path);

				args->sfd = sfd;
				acceptOnSocket(args);

				// Il figlio non esce mai da qui.
				break;
			} else {
				pids[i] = pid;
			}
		}

		// L'unico ad arrivare qui è il processo padre
		if (pid > 0) {
			sigset_t mask, oldMask;

			// Inizializzo la maschera vuota
			if (sigemptyset(&mask) != 0) {
				killForkedProcesses(pids, cfg->nconcurrent);
				handle_error("Error on init of sigmask");
			}

			// Aggiunto SIGHUP
			if (sigaddset(&mask, SIGHUP) != 0||
			    sigaddset(&mask, SIGCHLD) != 0 ||
			    sigaddset(&mask, SIGINT) != 0 ||
			    sigaddset(&mask, SIGTERM) != 0 ||
			    sigaddset(&mask, SIGKILL) != 0) {
				killForkedProcesses(pids, cfg->nconcurrent);
				handle_error("Error on adding signals to sigmask");
			}

			/* Setto la maschera dei segnali */
			if (sigprocmask(SIG_BLOCK, &mask, &oldMask) != 0) {
				handle_error("Error setting threads signal mask");
			}

			/* Aspetto il comando. */
			int sig = 0;
			do {
				if (sigwait(&mask, &sig) != 0) {
					handle_error("Error on sigwait");
				}

				if (sig == SIGCHLD) {
					wait(NULL);  // Niente zombies!
				}
			} while (sig == SIGCHLD);

			/* Rimuovo SIGHUP */
			if (pthread_sigmask(SIG_SETMASK, &oldMask, NULL) != 0) {
				handle_error("Error unsetting threads signal mask");
			}

			// Uccido i figli
			killForkedProcesses(pids, cfg->nconcurrent);

			// Cleanup!
			free(pids);
			free(args);

			if (sig != SIGHUP) {
				cleanExit(sig);
			}

			return;
		}
	}
#else
	#define DRIVE_BUFFER 4
	void serverMultiProcesses(SOCKET* sfds, int sfdslen, struct config* cfg, char* argvPath) {
		// Setto il ctrl handler
		if (SetConsoleCtrlHandler((PHANDLER_ROUTINE) ctrlHandler, TRUE) == 0) {
			handle_error("Error setting console ctrl handler");
		}

		int len = sizeof(char) * (BUFFER_SIZE + strlen(PROCESSED_SERVER_EXE) + 2);
		char* sfdString = Calloc(1, len);

		STARTUPINFO si;
		PROCESS_INFORMATION *pi = Malloc(sizeof(PROCESS_INFORMATION));

		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);

		HANDLE* processHandles = Calloc(1, sizeof(HANDLE) * cfg->nconcurrent);

		// Costruiamo il path dell'eseguibile
		char* dir = Calloc(1, sizeof(char) * (strlen(argvPath) + 1));
		char* drive = Calloc(1, sizeof(char) * (DRIVE_BUFFER));

		_splitpath(argvPath, drive, dir, NULL, NULL);

		char* exePath = Malloc(sizeof(char) * (strlen(dir) + strlen(drive) + strlen(PROCESSED_SERVER_EXE)));

		_makepath(exePath, drive, dir, PROCESSED_SERVER_EXE, NULL);

		int i = 0;
		for (i = 0; i < cfg->nconcurrent; ++i) {
			SOCKET duplicatedSocket = 0;

			if (DuplicateHandle(GetCurrentProcess(),
							(LPHANDLE)sfds[i % sfdslen],
							GetCurrentProcess(),
							(LPHANDLE)&duplicatedSocket,
							0,
							TRUE,  // Socket can be inherited
							DUPLICATE_SAME_ACCESS) == 0) {
				handle_error("Error duplicating handle");
			}

			int n = -1;
			if ((n = snprintf(sfdString, len, "%s %u", PROCESSED_SERVER_EXE, duplicatedSocket) < 0
				|| n > len)) {
				handle_error("Error converting socket handle to string");
			}

			// Start the  child process.
			if (CreateProcess(
								exePath,   	// No module name (use command line)
								sfdString,        // Command line
								NULL,           // Process handle not inheritable
								NULL,           // Thread handle not inheritable
								TRUE,          // Set handle inheritance to FALSE
								0,              // No creation flags
								NULL,           // Use parent's environment block
								NULL,           // Use parent's starting directory
								&si,            // Pointer to STARTUPINFO structure
								pi )           // Pointer to PROCESS_INFORMATION structure
			== FALSE) {
				handle_error("Error creating process");
			}

			processHandles[i] = pi->hProcess;
			closesocket(duplicatedSocket);
		}

		free(pi);

		free(drive);
		free(dir);
		free(exePath);

		HANDLE ctrlHandle = CreateEvent(NULL,
									   TRUE,
									   FALSE,
									   CTRL_HANDLER_NAME);
		if (ctrlHandle == NULL) {
			handle_error("Error while creating event");
		}

		// Aspetto l'evento di Ctrl+C
		if (WaitForSingleObject(ctrlHandle, INFINITE) == WAIT_FAILED) {
			handle_error("Error while waiting for console event");
		}

		// Resetto l'evento
		if (ResetEvent(ctrlHandle) == 0) {
			handle_error("Error while resetting event");
		}

		// Disattivo il precedente control handler
		if (SetConsoleCtrlHandler((PHANDLER_ROUTINE) ctrlHandler, FALSE) == 0) {
			handle_error("Error setting console ctrl handler");
		}

		// Ignoro il console control handler
		if (SetConsoleCtrlHandler(NULL, TRUE) == 0) {
			handle_error("Error ignoring console ctrl handler");
		}

		// Uccido i processi
		killCreatedProcesses(processHandles, cfg->nconcurrent);

		if (SetConsoleCtrlHandler(NULL, FALSE) == 0) {
			handle_error("Error removing console ctrl handler");
		}

		free(processHandles);
		free(sfdString);

		return;
	}
#endif
