#ifndef __FF_STDIO_H__
#define __FF_STDIO_H__

#include <errno.h>

#define ffconfigMAX_FILENAME 64

#define ff_fopen(file, mode)                  httpdfs_fopen(file, mode)
#define ff_fclose(stream)                     httpdfs_fclose(stream)
#define ff_fread(buffer, size, items, stream) httpdfs_fread(buffer, size, items, stream)

typedef struct _FF_FILE
{
	uint8_t *pucBuffer;				/* A buffer for providing fast unaligned access. */
	uint32_t ulFileSize;			/* File's Size. */
	uint32_t ulFilePointer;			/* Current Position Pointer. */
} FF_FILE;

#define FF_FindData_t BaseType_t /* Used for FTP. Suppress Error. */

#define stdioGET_ERRNO() ENOENT

FF_FILE *httpdfs_fopen( const char *pcFile, const char *pcMode);
int httpdfs_fclose(FF_FILE *pxStream);
size_t httpdfs_fread(void *pvBuffer, size_t xSize, size_t xItems, FF_FILE *pxStream);

#endif /* __FF_STDIO_H__ */
