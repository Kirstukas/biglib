#ifndef BIGLIB_H
#define BIGLIB_H

#include <stdint.h>

enum {
	BIGLIB_ERR_NOT_SUPPORTED=1,
	BIGLIB_ERR_INVALID_FILE,
	BIGLIB_ERR_FAILED_TO_OPEN_FILE,
	BIGLIB_ERR_FILENAME_TAKEN,
	BIGLIB_ERR_FILENAME_INVALID,
	BIGLIB_ERR_ARGUMMENT_MISSING,
	BIGLIB_ERR_ALLOC_FAIL
};

enum {
	BIGLIB_TYPE_UNKNOWN,
	BIGLIB_TYPE_BYTE_ARRAY,
	BIGLIB_TYPE_STREAM,
	BIGLIB_TYPE_FILENAME,
	BIGLIB_TYPE_STREAM_AREA
};

typedef struct {
	uint32_t size;
	uint32_t location;
	FILE* stream;
}biglib_source_stream_area;

typedef struct {
	uint32_t size;
	unsigned char* data;
}biglib_source_byte_array;

typedef struct {
	char* name;
	unsigned int type;
	void* source;
}biglib_file;

typedef struct {
	FILE* stream;
	uint32_t amount;
	biglib_file** files;
}biglib_big;

biglib_big* biglib_open(FILE* stream);
biglib_big* biglib_openFile(const char* filename);
#ifndef _WIN32
biglib_big* biglib_openMem(const void* data, size_t data_len);
#endif

biglib_big* biglib_read(FILE* stream);
biglib_big* biglib_readFile(const char* filename);
#ifndef _WIN32
biglib_big* biglib_readMem(const void* data, size_t data_len);
#endif

unsigned char* biglib_get(biglib_big* big, const char* name, size_t* size);

int biglib_write(biglib_big* big, FILE* stream);
int biglib_writeFile(biglib_big* big, const char* filename);
#ifndef _WIN32
unsigned char* biglib_writeMem(biglib_big* big);
#endif

biglib_big* biglib_create();
int biglib_addFile(biglib_big* big, const char* name, const char* filename);
int biglib_addData(biglib_big* big, const char* name, const unsigned char* data, size_t size);
int biglib_addStream(biglib_big* big, const char* name, FILE* stream);
int biglib_addStreamArea(biglib_big* big, const char* name, FILE* stream, unsigned long int location, size_t size);

int biglib_remove(biglib_big* big, const char* name);
int biglib_replaceFile(biglib_big* big, const char* name, const char* filename);
int biglib_replaceData(biglib_big* big, const char* name, const unsigned char* data, size_t size);
int biglib_replaceStream(biglib_big* big, const char* name, FILE* stream);
int biglib_replaceStreamArea(biglib_big* big, const char* name, FILE* stream, unsigned long int location, size_t size);

int biglib_sort(biglib_big* big);
int biglib_exists(biglib_big* big, const char* name);
int biglib_index(biglib_big* big, const char* name);

int biglib_freeFile(biglib_file* file);
int biglib_free(biglib_big* big);

int biglib_getError(char* log, int logLen);

#endif
