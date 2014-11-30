CFLAGS=-g -Wall -Isrc $(OPTDEFINE) $(OPTFLAGS)

SOURCES=$(wildcard src/*.c)
SERVER_SOURCES=$(wildcard src/Server/*.c)
CLIENT_SOURCES=$(wildcard src/Client/*.c)

OBJECTS=$(patsubst %.c,%.o,$(SOURCES))

LIB=build/libProtocol.a

#-DDEBUG is for debug, -DDAEMON is to launch tinyDB as a daemon under Unix/Linux
OPTDEFINE=-DDEBUG

ifeq ($(OS),Windows_NT)
#Windows stuff
OPTFLAGS=-D_WIN32_WINNT='0x0501'
OPTLIBS=-lws2_32
#If you want to cross compile, point here with your Windows (cross) compiler
# CC=/usr/local/gcc-4.8.0-qt-4.8.4-for-mingw32/win32-gcc/bin/i586-mingw32-gcc
# AR=/usr/local/gcc-4.8.0-qt-4.8.4-for-mingw32/win32-gcc/bin/i586-mingw32-ar
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

# Library, client and server
ifeq ($(OS),Windows_NT)
all: $(LIB) $(CLIENT) $(SERVER) $(PROCESSEDSERVER)
else
all: $(LIB) $(CLIENT) $(SERVER)
endif

# Client: library and subfiles
$(CLIENT): $(LIB) $(CLIENT_SOURCES)
	$(CC) $(CFLAGS) -o bin/$@ $(CLIENT_SOURCES) -lProtocol -Lbuild $(OPTLIBS)

# Server: library and subfiles
$(SERVER): $(LIB) $(SERVER_SOURCES)
	$(CC) $(CFLAGS) -o bin/$@ $(SERVER_SOURCES) -lProtocol -Lbuild $(OPTLIBS)

$(PROCESSEDSERVER): $(LIB) $(PROCESSED_SERVER_SOURCES) $(SERVER_SOURCES)
	$(CC) $(CFLAGS) -o bin/$@ $(PROCESSED_SERVER_SOURCES) -lProtocol -Lbuild $(OPTLIBS)

# Default rule for every .c file
%.o : %.c
	$(CC) $(CFLAGS) -o $@ -c $< $(OPTLIBS)

# Build the library
$(LIB): build $(OBJECTS)
	$(AR) rcs $@ $(OBJECTS)

.PHONY: clean build

build:
	@mkdir -p build
	@mkdir -p bin

clean:
	$(RM) bin build $(OBJECTS)
