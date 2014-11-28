CFLAGS=-g -Wall -Isrc $(OPTDEFINE) $(OPTFLAGS)

SOURCES=$(wildcard src/*.c)
SERVER_SOURCES=$(wildcard src/Server/*.c)
CLIENT_SOURCES=$(wildcard src/Client/*.c)

OBJECTS=$(patsubst %.c,%.o,$(SOURCES))

LIB=build/libProtocol.a

#-DDEBUG for debug flag, -DDAEMON to launch as daemon in Unix/Linux
OPTDEFINE=-DDEBUG

ifeq ($(OS),Windows_NT)
#Windows stuff
OPTFLAGS=-D_WIN32_WINNT='0x0501'
OPTLIBS=-lws2_32
#Point here with your Windows (cross) compiler
CC=/usr/local/gcc-4.8.0-qt-4.8.4-for-mingw32/win32-gcc/bin/i586-mingw32-gcc
AR=/usr/local/gcc-4.8.0-qt-4.8.4-for-mingw32/win32-gcc/bin/i586-mingw32-ar
RM=rm -rf
CLIENT=Client.exe
SERVER=Server.exe
PROCESSEDSERVER=ProcessedServer.exe
PROCESSED_SERVER_SOURCES=$(wildcard src/Server/ProcessedServer/*.c)
else
#Unix stuff
OPTFLAGS=-pthread
OPTLIBS=
RM=rm -rf
CLIENT=Client
SERVER=Server
endif

ifeq ($(OS),Windows_NT)
# Per tutti ho bisogno di libreria, client e server
all: $(LIB) $(CLIENT) $(SERVER) $(PROCESSEDSERVER)
else
all: $(LIB) $(CLIENT) $(SERVER)
endif

# Per il client ho bisogno di libreria, e file del client
$(CLIENT): $(LIB) $(CLIENT_SOURCES)
	$(CC) $(CFLAGS) -o bin/$@ $(CLIENT_SOURCES) -lProtocol -Lbuild $(OPTLIBS)

# Per il server ho bisogno di libreria, e file del server
$(SERVER): $(LIB) $(SERVER_SOURCES)
	$(CC) $(CFLAGS) -o bin/$@ $(SERVER_SOURCES) -lProtocol -Lbuild $(OPTLIBS)

$(PROCESSEDSERVER): $(LIB) $(PROCESSED_SERVER_SOURCES) $(SERVER_SOURCES)
	$(CC) $(CFLAGS) -o bin/$@ $(PROCESSED_SERVER_SOURCES) -lProtocol -Lbuild $(OPTLIBS)

# Regola di default per ogni .c
%.o : %.c
	$(CC) $(CFLAGS) -o $@ -c $< $(OPTLIBS)

# Costruisco la librera
$(LIB): build $(OBJECTS)
	$(AR) rcs $@ $(OBJECTS)

.PHONY: clean build check

build:
	@mkdir -p build
	@mkdir -p bin

clean:
	$(RM) bin build $(OBJECTS)
