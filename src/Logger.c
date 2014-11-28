#include "Logger.h"
#include "Wrapper.h"
#include "Utils.h"
#include "dbg.h"

#include <time.h>
#include <string.h>

#ifndef _WIN32
	#include <arpa/inet.h>
	#include <netdb.h>
#else
	#include <winsock2.h>
	#include <ws2tcpip.h>
#endif

#define TIME_STR_LEN 100
#define ADDRESS_LEN 30
#define TOTAL_LEN 200

void strupper(char *s) {
    while (*s) {
        if ((*s >= 'a' ) && (*s <= 'z')) *s -= ('a'-'A');
        s++;
    }
}

void logAppend(char* logpath, int code, struct sockaddr_storage *managedSocketAddress, int size, struct url *url) {
	FILE* logstream;
	if ((logstream = fopen(logpath, "a")) == NULL) {
		handle_error("Error opening log file");
	}

	char* logString = Calloc(1, TOTAL_LEN);
	time_t epochTime = -1;

	if (time(&epochTime) == -1) {
		handle_error("Error while getting epoch time");
	}

	struct tm *calTime = localtime(&epochTime);
	if (calTime == NULL) {
		log_err("Error while getting local time");
	}

	char* timeString = Calloc(1, TIME_STR_LEN * sizeof(char));

	if (strftime(timeString, TIME_STR_LEN * sizeof(char), "[%d/%b/%Y:%H:%M:%S %z]", calTime) == 0) {
		log_err("Error while converting time to string");
	}

	char* address = Calloc(1, ADDRESS_LEN * sizeof(char));
	getnameinfo((struct sockaddr *)managedSocketAddress,
	          	size,
				address,
				ADDRESS_LEN * sizeof(char),
				NULL,
				0,
				0);

	char* upperCommand = strdup(url->command);
	strupper(upperCommand);
	sprintf(logString, "%s %s \"%s %s\" %d\n", address, timeString, upperCommand, url->argument, code);

	if (fwrite(logString, sizeof(char), strlen(logString), logstream) != strlen(logString))	{
		log_err("Incomplete write on log file.");
	}

	free(upperCommand);
	free(logString);
	free(timeString);
	free(address);

	fclose(logstream);
	return;
}
