#include "Config.h"
#include "Utils.h"
#include "Wrapper.h"
#include "dbg.h"

#include <stdio.h>
#include <string.h>

#define ROOT "/tmp"
#define LOG "dblog"
#define PROCESS 1
#define NCONCURRENT 8
#define BUFFER_SIZE 50

struct config* loadConfiguration() {
	FILE* fstream;
	if ((fstream = fopen(CONFIG_FILE, "r")) == NULL) {
		// Si potrebbero anche utilizzare valori di default.
		log_err("Error opening config file; using default value");
	}

	struct config *cfg = Calloc(1, sizeof(struct config));

	// Inizializzazione della struct ai valori di default
	cfg->port 			= strdup(PORT);
	cfg->rootdir 		= strdup(ROOT);
	cfg->logpath 		= strdup(LOG);
	cfg->process 		= PROCESS;
	cfg->nconcurrent 	= NCONCURRENT;

	if (fstream != NULL) {
		char *toFree, *buffer;
		buffer = toFree = Malloc(BUFFER_SIZE);

		while (fstream != NULL && !(feof(fstream))) {
			buffer = toFree;

			if (fgets(toFree, BUFFER_SIZE, fstream) != NULL) {
				char* key = strsep(&buffer, "\t");
				char* value = strsep(&buffer, "\n");

				if (strcmp(key, "port") == 0) {
					cfg->port = strdup(value);
					// log_info("Port: %s", cfg->port);
				} else if (strcmp(key, "rootdir") == 0) {
					cfg->rootdir = strdup(value);
					// log_info("Rootdir: %s", cfg->rootdir);
				} else if (strcmp(key, "logpath") == 0) {
					cfg->logpath = strdup(value);
					// log_info("Logpath: %s", cfg->logpath);
				} else if (strcmp(key, "process") == 0) {
					cfg->process = atoi(value);
					// log_info("Process: %d", cfg->process);
				} else if (strcmp(key, "nconcurrent") == 0) {
					cfg->nconcurrent = atoi(value);
					// log_info("Nconcurrent: %d", cfg->nconcurrent);
				} else {
					// log_info("Unknown config entry.");
				}
			} else {
				// log_info("Invalid line. (Reached EOF?)");
			}
		}

		free(toFree);
		fclose(fstream);
	}

	return cfg;
}

void freeConfig(struct config* cfg) {
	/* Libero la struct utilizzata per la configurazione. */
	free(cfg->rootdir);
	free(cfg->logpath);
	free(cfg->port);
	free(cfg);

	return;
}
