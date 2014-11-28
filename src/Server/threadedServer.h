#ifndef __threadedServer_h__
#define __threadedServer_h__

#include "../Config.h"
#include "../Utils.h"
#include "../Protocol.h"
#include "Server.h"

void serverMultiThreads(SOCKET_TYPE* sfds, int sfdslen, struct config* cfg);

#endif
