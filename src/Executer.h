#ifndef __Executer_h__
#define __Executer_h__

#include "Protocol.h"
#include "Config.h"

#include <stdint.h>

int checkIdentifier(char* identifier, char* rootdir);
int checkPath(char* path);

int executeServerCommand(int msfd, struct url *url, struct config *cfg);

int makeDir(char* identifierPath, char* path);
int deletefd(char* identifierPath, char* path);
int movefd(char* identifierPath, char* old, char* new);
int putf(int msfd, char* identifierPath, char* argument, char* optionalArgument, char* mode);
int getf(int msfd, char* identifierPath, char* argument);

char* createFullPath(char* firsts, char* seconds);

int sendReturnCode(int msfd, uint32_t returnCode);
#endif
