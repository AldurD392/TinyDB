# tinyDB #
A tiny, client/server implementation of a Dropbox-like service, entirely written in C.

## Features ##
* Natively cross-platform (Unix/Linux, BSD and Windows)
* Multi-thread or multi-process server architecture
* User name-space separation
* Exclusive file access control -- mutual exclusion
* Configurable from CLI or configuration file
* Common log format support
* Network level independent (IPv4, IPv6)
* Domain name resolution
* Unix/Linux daemon mode

## Installation ##
Simply clone this repository and run `make`.
You'll find the Client and Server executables under the `bin` directory.

### Windows support and cross-compilation ###
Windows if fully supported __natively__ (no cygwin), but you could need to adapt the `Makefile` to your build environment.
You could even cross-compile binaries, by setting the proper `$(CC)` and by running `make OS=Windows_NT`.

### Installation flags ###
You can specify the `DAEMON` and `DEBUG` flags in the `Makefile` to produce, respectively, a daemon under Unix/Linux and to access debugging informations.

## Run ##
Simply create the [server root directory](#configuration), a user identifier (`k9Pg4mweRm` as instance) and a home directory for your user (i.e. root/`k9Pg4mweRm`).
Then, launch the server and start executing commands from the client.

## Supported client commands ##
The list of supported client commands.

**MKDIR** - Folder creation:
```
mydb://www.foo.bar/k9Pg4mweRm/mkdir/newdir
```

**PUT** - File upload:
```
mydb://www.foo.bar/k9Pg4mweRm/put/localPath{remotePath}
```

**PUTA** - File append:
```
mydb://www.foo.bar/k9Pg4mweRm/puta/localPath{remotePath}
```

**MOVE** - File rename/move:
```
mydb://www.foo.bar/k9Pg4mweRm/move/oldPath{newPath}
```

**DELE** - File delete:
```
mydb://www.foo.bar/k9Pg4mweRm/dele/filePath
```

**GET** - File download:
```
mydb://www.foo.bar/k9Pg4mweRm/get/remotePath{localPath}
```

## Configuration ##
You can check `Config.cfg` for a configuration sample.
You can specify the `port ` to which the server will be listening to, the `root` of the server (the one in which the user homes will be put) and the `logpath`.
Then you can specify the number of process to spawn and the number of thread (respectively `process` and `nconcurrent`).

## Return codes ##
A list of the possible return codes can be found in the `Errors.h` file.

## The authors ##
Adriano Di Luzio & Danilo Francati
