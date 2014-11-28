#ifndef __Errors_h__
#define __Errors_h__

#define ALLOK 200 // tutto ok
#define TEMP_UNAVAIBLE 300 // Il file è temporaneamente inaccesibile (lock?)
#define INVALID_PATH 400 // Il path non esiste o è malformato
#define CANNOT_SAVE_FILE 401 // Il file non può essere salvato
#define DIRECTORY_NOT_EMPTY 402 // Directory non vuota
#define PERMISSION_DENIED 403 // Permessi sbagliati
#define FILE_EXISTS 405 // Il file esiste già
#define GENERIC_ERROR 410 // Errore generico
#define BAD_IDENTIFIER 501 // Identificativo errato
#define MALFORMED_COMMAND 502 // Comando malformato

int parseErrno(int errorNumber);

#endif
