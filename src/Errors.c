#include "Errors.h"
#include <errno.h>

int parseErrno(int errorNumber)
{
	switch (errorNumber)
	{
	    case ENOENT:
	    case ENOTDIR:
	    case EINVAL:
	        return INVALID_PATH;

	    case EEXIST:
	    	return FILE_EXISTS;

	    case ENOTEMPTY:
	        return DIRECTORY_NOT_EMPTY;

	    case EACCES:
	    case EPERM:
#ifndef _WIN32
	    case EDQUOT:
#endif
	    	return CANNOT_SAVE_FILE;

	    default:
	        return GENERIC_ERROR;
	}
}
