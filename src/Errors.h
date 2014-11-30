#ifndef __Errors_h__
#define __Errors_h__

#define ALLOK 200
#define TEMP_UNAVAIBLE 300 // The file is temporarily non-available (maybe because of lock)
#define INVALID_PATH 400 // The specified path doesn't exists or it is malformed
#define CANNOT_SAVE_FILE 401 // The file cannot be saved
#define DIRECTORY_NOT_EMPTY 402 // Directory is not empty
#define PERMISSION_DENIED 403 // Permission are wrong
#define FILE_EXISTS 405
#define GENERIC_ERROR 410
#define BAD_IDENTIFIER 501 // Bad user identifier
#define MALFORMED_COMMAND 502 // Malformed command

int parseErrno(int errorNumber);

#endif
