#include "windowsProcessedServer.h"
#include "../../Config.h"
#include "../../Utils.h"
#include "../../Wrapper.h"
#include "../../Protocol.h"
#include "../../dbg.h"

#include <stdlib.h>
#include <limits.h>

struct acceptArgs* args;
struct config *cfg;

BOOL ctrlHandler(DWORD fwdCtrlType) {
	if (fwdCtrlType == CTRL_C_EVENT) {
		closesocket(args->sfd);

		free(args);
		free(cfg);

		if (WSACleanup() != 0) {
			handle_error("Error cleaning WSA");
		}

		ExitProcess(0);
		return TRUE;
	} else {
		return FALSE;
	}
}

int main(int argc, char const *argv[]) {
	/* Questa funzione, verrà eseguita soltanto da il processo Windows,
	creato tramite la createProcess */

	SOCKET socketHandle = -1;
	if (argc < 2) {
		handle_error("Invalid socket handle");
	} else {
		socketHandle = strtol(argv[1], NULL, 10);
		if (socketHandle == LONG_MIN || socketHandle == LONG_MAX) {
			handle_error("Error converting socket to socketHandle");
		}
	}

	if (SetConsoleCtrlHandler((PHANDLER_ROUTINE) ctrlHandler, TRUE) == 0) {
		handle_error("Error setting console ctrl handler");
	}

	initWSA();

	cfg = loadConfiguration();

	args = Calloc(1, sizeof(struct acceptArgs));
	args->cfg = cfg;
	args->sfd = socketHandle;

	acceptOnSocket(args);  // Il processo non esce mai da qui

	return 0;
}
