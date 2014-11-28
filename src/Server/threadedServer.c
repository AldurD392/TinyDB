#include "threadedServer.h"
#include "../Wrapper.h"
#include "../Config.h"
#include "../Utils.h"
#include "../dbg.h"
#include "../Sync.h"
#include "../Protocol.h"
#include "Server.h"

#include <signal.h>

#ifndef _WIN32
	#include <pthread.h>
#else
	#if NTDDI_VERSION <= NTDDI_WIN7
		#include <Windows.h>
		#include <winbase.h>
	#else
		#include <Processthreadspi.h>
	#endif

	#include "Wincon.h"
#endif

#ifndef _WIN32
	void killCreatedThreads(pthread_t* threads, int len, struct acceptArgs** argss) {
		int i;
		for (i = 0; i < len; ++i) {
			if (pthread_cancel(threads[i]) != 0) {
				handle_error("Error while asking thread to terminate");
			}
		}
		for (i = 0; i < len; ++i) {
			if (pthread_join(threads[i], NULL) != 0) {
				handle_error("Error while waiting for threads to terminate");
			}
			free(argss[i]);
		}

		free(argss);
	}

	void serverMultiThreads(int* sfds, int sfdslen, struct config* cfg) {
		pthread_t *pthreads = Malloc(sizeof(pthread_t) * cfg->nconcurrent);
		pthread_attr_t *pthreadsAttr = Malloc(sizeof(pthread_attr_t));

		if (pthread_attr_init(pthreadsAttr) != 0) {
			handle_error("Error initializing threads attributes struct");
		}

		int i;
		struct acceptArgs** argss = Malloc(cfg->nconcurrent * sizeof(struct acceptArgs*));
		for (i = 0; i < cfg->nconcurrent; ++i) {
			argss[i] = Calloc(1, sizeof(struct acceptArgs));
			argss[i]->cfg = cfg;
			argss[i]->sfd = sfds[i % sfdslen];

			if (pthread_create(&pthreads[i], NULL, (void *(*)(void *))&acceptOnSocket, argss[i]) != 0) {
				handle_error("Error while creating thread");
			}
		}

		if (pthread_attr_destroy(pthreadsAttr) != 0) {
			log_warn("Error destroying threads attributes struct");
		}
		free(pthreadsAttr);

		sigset_t mask, oldMask;

		// Inizializzo la maschera vuota
		if (sigemptyset(&mask) != 0) {
			killCreatedThreads(pthreads, cfg->nconcurrent, argss);
			handle_error("Error on init of sigmask");
		}

		// Aggiunto SIGHUP
		if (sigaddset(&mask, SIGHUP) != 0||
		    sigaddset(&mask, SIGINT) != 0 ||
		    sigaddset(&mask, SIGTERM) != 0 ||
		    sigaddset(&mask, SIGKILL) != 0) {
			killCreatedThreads(pthreads, cfg->nconcurrent, argss);
			handle_error("Error on adding signals to sigmask");
		}

		/* Setto la maschera dei segnali */
		if (pthread_sigmask(SIG_BLOCK, &mask, &oldMask) != 0) {
			handle_error("Error setting threads signal mask");
		}

		/* Aspetto il comando. */
		int sig = 0;
		if (sigwait(&mask, &sig) != 0) {
			handle_error("Error on sigwait");
		}

		/* Rimuovo SIGHUP */
		if (pthread_sigmask(SIG_SETMASK, &oldMask, NULL) != 0) {
			handle_error("Error unsetting threads signal mask");
		}

		// Uccido i figli
		killCreatedThreads(pthreads, cfg->nconcurrent, argss);

		// Cleanup!
		free(pthreads);

		if (sig != SIGHUP) {
			cleanExit(sig);
		}

		return;
	}
#else
	void killCreatedThreads(HANDLE* threads, int len, SOCKET* sfds, int sfdslen, struct acceptArgs** argss) {
		// Creo l'evento per i thread
		HANDLE threadHandle = CreateEvent(NULL,
		                               TRUE,
		                               FALSE,
		                               THREAD_HANLDER_NAME);

		// Controllo il buon fine della creazione
		if (threadHandle == NULL) {
			handle_error("Error while creating event");
		}

		// Setto l'evento
		if (SetEvent(threadHandle) == 0) {
			handle_error("Error while setting event");
		}

		// Chiudo i file descriptors
		int i = 0;
		for (i = 0; i < sfdslen; ++i) {
			if (closesocket(sfds[i]) == SOCKET_ERROR) {
				log_err("Error while closing socket");
			}
		}

		// Aspetto che tutti i figli muoiano
		if (WaitForMultipleObjects(len, threads, TRUE, INFINITE) == WAIT_FAILED) {
			handle_error("Error while waiting for threads");
		}

		// Resetto l'evento
		if (ResetEvent(threadHandle) == 0) {
			handle_error("Error while setting event");
		}
	}

	void serverMultiThreads(SOCKET* sfds, int sfdslen, struct config* cfg) {
		// Setto il ctrl handler
		if (SetConsoleCtrlHandler((PHANDLER_ROUTINE) ctrlHandler, TRUE) == 0) {
			handle_error("Error setting console ctrl handler");
		}

		int i;
		struct acceptArgs** argss = Malloc(cfg->nconcurrent * sizeof(struct acceptArgs*));

		// Creo i thread
		HANDLE* threads = Malloc(sizeof(HANDLE) * cfg->nconcurrent);
		for (i = 0; i < cfg->nconcurrent; ++i) {
			argss[i] = Calloc(1, sizeof(struct acceptArgs));
			argss[i]->cfg = cfg;
			argss[i]->sfd = sfds[i % sfdslen];

			HANDLE threadHandle = NULL;
			if ((threadHandle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&acceptOnSocket, argss[i], 0, NULL)) == NULL) {
				handle_error("Error while creating thread");
			}
			threads[i] = threadHandle;
		}

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

		// Uccido (bruscamente) i threads
		killCreatedThreads(threads, cfg->nconcurrent, sfds, sfdslen, argss);

		// Resetto l'evento
		if (ResetEvent(ctrlHandle) == 0) {
			handle_error("Error while resetting event");
		}

		// Rimuovo il console control handler
		if (SetConsoleCtrlHandler((PHANDLER_ROUTINE) ctrlHandler, FALSE) == 0) {
			handle_error("Error setting console ctrl handler");
		}

		return;
	}
#endif
