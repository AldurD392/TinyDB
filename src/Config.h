#ifndef __Config_h__

#define __Config_h__
#define CONFIG_FILE "Config.cfg"
#define IDENTIFIER_LEN 10
#define BIG_BUFFER_SIZE 1000
#define PORT "6010"

struct config {
	char* port;
	char* rootdir;
	char* logpath;
	unsigned int process;
	unsigned int nconcurrent;
};

struct config* loadConfiguration();
void freeConfig(struct config* cfg);

#endif
