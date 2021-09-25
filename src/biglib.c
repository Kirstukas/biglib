#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <biglib.h>

#define reverseBytes(b) ((b >> 24) | (b >> 16 << 24 >> 16) | (b << 16 >> 24 << 16) | (b << 24))

#define DEBUG_LINE_CHECK printf("Debug print at line \x1B[96m%d\x1B[0m in \x1B[33m%s\x1B[0m\n", __LINE__, __FILE__);

static unsigned int err;



static char* readString(FILE* stream){
	uint32_t size = 512;
	char* string = malloc(size);
	uint32_t pos = 0;
	char ch;
	while (ch = getc(stream)){
		string[pos] = ch;
		pos++;
		if (pos>=size){
			size += 512;
			string = realloc(string, size);
		}
	}
	string[pos] = 0;
	if (pos+1<size){
		string = realloc(string, pos+1);
	}

	err = 0;
	return string;
}

biglib_big* biglib_open(FILE* stream){
	biglib_big* big = malloc(sizeof(biglib_big));

	big->stream = stream;

	char BIGF[5] = {0,0,0,0,0};
	uint32_t amount;

	fread(BIGF, 1, 4, stream);
	if (strcmp(BIGF, "BIGF")){
		err = BIGLIB_ERR_INVALID_FILE;
		return NULL;
	}

	fseek(stream, 4, SEEK_CUR);
	fread(&amount, sizeof(uint32_t), 1, stream);
	amount = reverseBytes(amount);

	big->amount = amount;
	big->files = malloc(sizeof(void*)*amount);

	fseek(stream, 4, SEEK_CUR);
	for (uint32_t i = 0; i < amount; i++){
		biglib_source_stream_area* file = malloc(sizeof(biglib_source_stream_area));

		file->stream = stream;
		fread(&file->location, sizeof(uint32_t), 1, stream);
		fread(&file->size, sizeof(uint32_t), 1, stream);

		file->location = reverseBytes(file->location);
		file->size = reverseBytes(file->size);

		big->files[i] = malloc(sizeof(biglib_file));
		big->files[i]->name = readString(stream);
		big->files[i]->type = BIGLIB_TYPE_STREAM_AREA;
		big->files[i]->source = file;
		big->files[i]->cursor = 0;
	}

	err = 0;
	return big;
}

biglib_big* biglib_openFile(const char* filename){
	if (!filename){
		err = BIGLIB_ERR_ARGUMMENT_MISSING;
		return NULL;
	}

	FILE* file = fopen(filename, "rb");

	if (!file){
		err = BIGLIB_ERR_FAILED_TO_OPEN_FILE;
		return NULL;
	}

	biglib_big* big = biglib_open(file);
	if (!big){
		fclose(file);
		return NULL;
	}
	err = 0;
	return big;
}

#ifndef _WIN32
biglib_big* biglib_openMem(const void* data, size_t data_len){
	if (!data){
		err = BIGLIB_ERR_ARGUMMENT_MISSING;
		return NULL;
	}

	FILE* file = fmemopen((void*)data, data_len, "rb");

	if (!file){
		err = BIGLIB_ERR_FAILED_TO_OPEN_FILE;
		return NULL;
	}

	biglib_big* big = biglib_open(file);
	if (!big){
		fclose(file);
		return NULL;
	}

	err = 0;
	return big;
}
#endif





static biglib_big* readBig(biglib_big* big){
	for (uint32_t i = 0; i < big->amount; i++){
		biglib_file* file = big->files[i];
		if (file->type == BIGLIB_TYPE_STREAM_AREA){
			biglib_source_stream_area* streamArea = (biglib_source_stream_area*)(file->source);

			biglib_source_byte_array* newFile = malloc(sizeof(biglib_source_byte_array));
			newFile->size = streamArea->size;
			newFile->data = malloc(streamArea->size);
			fseek(streamArea->stream, streamArea->location, SEEK_SET);
			fread(newFile->data, 1, streamArea->size, streamArea->stream);

			file->type = BIGLIB_TYPE_BYTE_ARRAY;
			file->source = newFile;

			free(streamArea);
		}
	}

	err = 0;
	return big;
}

biglib_big* biglib_read(FILE* stream){
	biglib_big* big = biglib_open(stream);
	if (big){
		readBig(big);
	}

	err = 0;
	return big;
}

biglib_big* biglib_readFile(const char* filename){
	biglib_big* big = biglib_openFile(filename);
	if (big){
		readBig(big);
		fclose(big->stream);
		big->stream = NULL;
	}

	err = 0;
	return big;
}

#ifndef _WIN32
biglib_big* biglib_readMem(const void* data, size_t data_len){
	biglib_big* big = biglib_openMem(data, data_len);
	if (big){
		readBig(big);
		fclose(big->stream);
		big->stream = NULL;
	}

	err = 0;
	return big;
}
#endif



size_t biglib_getSize(biglib_big* big, const char* name){
	if (!big || !name){
		err = BIGLIB_ERR_ARGUMMENT_MISSING;
		return 0;
	}
	if (!biglib_exists(big, name)){
		err = BIGLIB_ERR_FILENAME_INVALID;
		return 0;
	}

	biglib_file* file = big->files[biglib_index(big, name)];
	switch (file->type){
		size_t size;
		long int pos;
		FILE* stream;
		case BIGLIB_TYPE_BYTE_ARRAY:
			return ((biglib_source_byte_array*)file->source)->size;
			break;
		case BIGLIB_TYPE_STREAM:
			pos = ftell(file->source);
			fseek(file->source, 0, SEEK_END);
			size = ftell(file->source);
			fseek(file->source, pos, SEEK_SET);
			return size;
			break;
		case BIGLIB_TYPE_FILENAME:
			stream = fopen(file->source, "rb");
			if (!stream)
				return 0;
			fseek(stream, 0, SEEK_END);
			size = ftell(stream);
			fclose(stream);
			return size;
			break;
		case BIGLIB_TYPE_STREAM_AREA:
			return ((biglib_source_stream_area*)file->source)->size;
			break;
	}
	return 0;
}

int biglib_fseek(biglib_big* big, const char* name, long int offset, int origin){
	if (!big || !name){
		err = BIGLIB_ERR_ARGUMMENT_MISSING;
		return 0;
	}
	if (!biglib_exists(big, name)){
		err = BIGLIB_ERR_FILENAME_INVALID;
		return 0;
	}

	biglib_file* file = big->files[biglib_index(big, name)];
	size_t size = biglib_getSize(big, name);
	

	switch (origin){
		case SEEK_SET:
			file->cursor = offset;
			break;
		case SEEK_CUR:
			file->cursor += offset;
			break;
		case SEEK_END:
			file->cursor = size+offset;
			break;
	}
	if (file->cursor<0)
		file->cursor = 0;
	else if (file->cursor > size)
		file->cursor = size;
}

int biglib_fgetc(biglib_big* big, const char* name){
	if (!big || !name){
		err = BIGLIB_ERR_ARGUMMENT_MISSING;
		return 0;
	}
	if (!biglib_exists(big, name)){
		err = BIGLIB_ERR_FILENAME_INVALID;
		return 0;
	}

	biglib_file* file = big->files[biglib_index(big, name)];
	if (file->cursor >= biglib_getSize(big, name))
		return EOF;
	int ch = 0;
	FILE* stream;
	long int pos;
	switch (file->type){
		case BIGLIB_TYPE_BYTE_ARRAY:
			ch = ((biglib_source_byte_array*)file->source)->data[file->cursor];
			break;
		case BIGLIB_TYPE_STREAM:
			stream = file->source;
			pos = ftell(stream);
			fseek(stream, file->cursor, SEEK_SET);
			ch = fgetc(stream);
			fseek(stream, pos, SEEK_SET);
			break;
		case BIGLIB_TYPE_FILENAME:
			stream = fopen(file->source, "rb");
			fseek(stream, file->cursor, SEEK_SET);
			ch = fgetc(stream);
			fclose(stream);
			break;
		case BIGLIB_TYPE_STREAM_AREA:
			stream = ((biglib_source_stream_area*)file->source)->stream;
			pos = ftell(stream);
			fseek(stream, ((biglib_source_stream_area*)file->source)->location+file->cursor, SEEK_SET);
			ch = fgetc(stream);
			fseek(stream, pos, SEEK_SET);
			break;
	}
	file->cursor++;
	return ch;
}

char* biglib_fgets(biglib_big* big, const char* name, char* str, int count){
	if (!big || !name){
		err = BIGLIB_ERR_ARGUMMENT_MISSING;
		return NULL;
	}
	if (!biglib_exists(big, name)){
		err = BIGLIB_ERR_FILENAME_INVALID;
		return NULL;
	}

	biglib_file* file = big->files[biglib_index(big, name)];
	if (file->cursor >= biglib_getSize(big, name))
		return NULL;
	unsigned int n;
	FILE* stream;
	long int pos;
	int ch = 0;
	switch (file->type){
		case BIGLIB_TYPE_BYTE_ARRAY:
			for (n = 0; n < count-1; n++){
				ch = ((biglib_source_byte_array*)file->source)->data[file->cursor];
				file->cursor++;
				if (ch == '\n' || ch == EOF)
					break;
				str[n] = ((char)ch);
			}
			str[n] = '\0';
			break;
		case BIGLIB_TYPE_STREAM:
			stream = file->source;
			pos = ftell(stream);
			fseek(stream, file->cursor, SEEK_SET);
			for (n = 0; n < count-1; n++){
				ch = fgetc(stream);
				file->cursor++;
				if (ch == '\n' || ch == EOF)
					break;
				str[n] = ((char)ch);
			}
			str[n] = '\0';
			fseek(stream, pos, SEEK_SET);
			break;
		case BIGLIB_TYPE_FILENAME:
			stream = fopen(file->source, "rb");
			fseek(stream, file->cursor, SEEK_SET);
			for (n = 0; n < count-1; n++){
				ch = fgetc(stream);
				file->cursor++;
				if (ch == '\n' || ch == EOF)
					break;
				str[n] = ((char)ch);
			}
			str[n] = '\0';
			fclose(stream);
			break;
		case BIGLIB_TYPE_STREAM_AREA:
			stream = ((biglib_source_stream_area*)file->source)->stream;
			pos = ftell(stream);
			fseek(stream, ((biglib_source_stream_area*)file->source)->location+file->cursor, SEEK_SET);
			for (n = 0; n < count-1; n++){
				ch = fgetc(stream);
				file->cursor++;
				if (ch == '\n' || ch == EOF)
					break;
				str[n] = ((char)ch);
			}
			str[n] = '\0';
			fseek(stream, pos, SEEK_SET);
			break;
	}
	return str;
}

size_t biglib_fread(biglib_big* big, const char* name, void* buffer, size_t size, size_t count){
	if (!big || !name){
		err = BIGLIB_ERR_ARGUMMENT_MISSING;
		return 0;
	}
	if (!biglib_exists(big, name)){
		err = BIGLIB_ERR_FILENAME_INVALID;
		return 0;
	}

	biglib_file* file = big->files[biglib_index(big, name)];
	if (file->cursor >= biglib_getSize(big, name))
		return 0;
	size_t ns;
	size_t n;
	FILE* stream;
	long int pos;
	long int oldCursor = file->cursor;
	switch (file->type){
		case BIGLIB_TYPE_BYTE_ARRAY:
			for (n = 0; n < count; n++){
				for (ns = 0; ns < size; ns++){
					((char*)buffer)[n*size+ns] = ((biglib_source_byte_array*)file->source)->data[file->cursor];
					file->cursor++;
				}
			}
			break;
		case BIGLIB_TYPE_STREAM:
			stream = file->source;
			pos = ftell(stream);
			fseek(stream, file->cursor, SEEK_SET);
			for (n = 0; n < count; n++){
				for (ns = 0; ns < size; ns++){
					((char*)buffer)[n*size+ns] = fgetc(stream);
					file->cursor++;
				}
			}
			fseek(stream, pos, SEEK_SET);
			break;
		case BIGLIB_TYPE_FILENAME:
			stream = fopen(file->source, "rb");
			fseek(stream, file->cursor, SEEK_SET);
			for (n = 0; n < count; n++){
				for (ns = 0; ns < size; ns++){
					((char*)buffer)[n*size+ns] = fgetc(stream);
					file->cursor++;
				}
			}
			fclose(stream);
			break;
		case BIGLIB_TYPE_STREAM_AREA:
			stream = ((biglib_source_stream_area*)file->source)->stream;
			pos = ftell(stream);
			fseek(stream, ((biglib_source_stream_area*)file->source)->location+file->cursor, SEEK_SET);
			for (n = 0; n < count; n++){
				for (ns = 0; ns < size; ns++){
					((char*)buffer)[n*size+ns] = fgetc(stream);
					file->cursor++;
				}
			}
			fseek(stream, pos, SEEK_SET);
			break;
	}
	return file->cursor - oldCursor;
}



int biglib_write(biglib_big* big, FILE* stream){
	if (!big || !stream){
		err = BIGLIB_ERR_ARGUMMENT_MISSING;
		return 0;
	}

	uint32_t filesize = 0, datasize = 0, headersize = 24-1;

	void** source	= malloc(sizeof(void*)*big->amount);
	uint32_t* size	= malloc(sizeof(uint32_t)*big->amount);

	// Do math
	for (uint32_t i = 0; i < big->amount; i++){
		headersize += 9+strlen(big->files[i]->name);
		switch (big->files[i]->type){
			case BIGLIB_TYPE_BYTE_ARRAY:
				source[i] = ((biglib_source_byte_array*)(big->files[i]->source))->data;
				size[i]   = ((biglib_source_byte_array*)(big->files[i]->source))->size;
				break;
			case BIGLIB_TYPE_FILENAME:
				source[i] = fopen(big->files[i]->source, "rb");
				if (!source[i]){
					for (uint32_t n = 0; n < i; n++)
						if (big->files[i]->type == BIGLIB_TYPE_FILENAME)
							fclose(source[n]);

					free(source);
					free(size);
					err = BIGLIB_ERR_FAILED_TO_OPEN_FILE;
					return 0;
				}
				fseek(source[i], 0, SEEK_END);
				size[i] = ftell(source[i]);
				fseek(source[i], 0, SEEK_SET);
				break;
			case BIGLIB_TYPE_STREAM:
				source[i] = big->files[i]->source;
				fseek(source[i], 0, SEEK_END);
				size[i] = ftell(source[i]);
				fseek(source[i], 0, SEEK_SET);
				break;
			case BIGLIB_TYPE_STREAM_AREA:
				source[i] = ((biglib_source_stream_area*)(big->files[i]->source))->stream;
				size[i]   = ((biglib_source_stream_area*)(big->files[i]->source))->size;
				break;
		}
		datasize += size[i];
	}
	filesize = headersize + datasize;
	const uint32_t amount_r=reverseBytes(big->amount), headersize_r=reverseBytes(headersize);

	// Start writing
	fwrite("BIGF", 1, 4, stream);
	fwrite(&filesize, sizeof(uint32_t), 1, stream);
	fwrite(&amount_r, sizeof(uint32_t), 1, stream);
	fwrite(&headersize_r, sizeof(uint32_t), 1, stream);

	{
		uint32_t dataPos = headersize+1;
		for (uint32_t i = 0; i < big->amount; i++){
			uint32_t start_r = reverseBytes(dataPos), size_r = reverseBytes(size[i]);
			fwrite(&start_r, sizeof(uint32_t), 1, stream);
			fwrite(&size_r, sizeof(uint32_t), 1, stream);
			fwrite(big->files[i]->name, 1, strlen(big->files[i]->name)+1, stream);
			dataPos += size[i];
		}
	}
	fwrite("L255\0\0\0\0", 1, 8, stream);
	
	for (uint32_t i = 0; i < big->amount; i++){
		switch (big->files[i]->type){
			long int pos;
			case BIGLIB_TYPE_BYTE_ARRAY:
				fwrite(source[i], 1, size[i], stream);
				break;
			case BIGLIB_TYPE_FILENAME:
				for (size_t n = 0; n < size[i]; n++)
					fputc(fgetc(source[i]), stream);
				fclose(source[i]);
				break;
			case BIGLIB_TYPE_STREAM:
				pos = ftell(source[i]);
				fseek(source[i], 0, SEEK_SET);
				for (size_t n = 0; n < size[i]; n++)
					fputc(fgetc(source[i]), stream);
				fseek(source[i], pos, SEEK_SET);
				break;
			case BIGLIB_TYPE_STREAM_AREA:
				pos = ftell(source[i]);
				fseek(source[i], ((biglib_source_stream_area*)(big->files[i]->source))->location, SEEK_SET);
				for (size_t n = 0; n < size[i]; n++)
					fputc(fgetc(source[i]), stream);
				fseek(source[i], pos, SEEK_SET);
				break;
		}
	}

	free(source);
	free(size);

	return 1;
}

int biglib_writeFile(biglib_big* big, const char* filename){
	if (!big || !filename){
		err = BIGLIB_ERR_ARGUMMENT_MISSING;
		return 0;
	}
	FILE* file = fopen(filename, "wb");
	if (!file){
		err = BIGLIB_ERR_FAILED_TO_OPEN_FILE;
		return 0;
	}

	if (!biglib_write(big, file))
		return 0;

	fclose(file);
	err = 0;
	return 1;
}

#ifndef _WIN32
unsigned char* biglib_writeMem(biglib_big* big){
	if (!big){
		err = BIGLIB_ERR_ARGUMMENT_MISSING;
		return NULL;
	}
	unsigned char* data;
	size_t data_len;
	FILE* file = open_memstream((char**)(&data), &data_len);
	if (!file){
		err = BIGLIB_ERR_FAILED_TO_OPEN_FILE;
		return NULL;
	}

	if (!biglib_write(big, file)){
		free(data);
		return NULL;
	}

	fclose(file);
	err = 0;
	return data;
}
#endif





biglib_big* biglib_create(){
	biglib_big* big = malloc(sizeof(biglib_big));
	big->amount = 0;
	big->files = NULL;
	big->stream = NULL;
	err = 0;
	return big;
}

int biglib_addFile(biglib_big* big, const char* name, const char* filename){
	if (!big || !name || !filename){
		err = BIGLIB_ERR_ARGUMMENT_MISSING;
		return 0;
	}
	if (biglib_exists(big, name)){
		err = BIGLIB_ERR_FILENAME_TAKEN;
		return 0;
	}
	
	biglib_file* file = malloc(sizeof(biglib_file));
	file->name = malloc(strlen(name)+1);
	strcpy(file->name, name);
	file->type = BIGLIB_TYPE_FILENAME;
	file->source = malloc(strlen(filename)+1);
	file->cursor = 0;
	strcpy(file->source, filename);
	big->files = realloc(big->files, sizeof(void*)*(big->amount+1));
	big->files[big->amount++] = file;

	return 1;
}

int biglib_addData(biglib_big* big, const char* name, const unsigned char* data, size_t size){
	if (!big || !name || !data){
		err = BIGLIB_ERR_ARGUMMENT_MISSING;
		return 0;
	}
	if (biglib_exists(big, name)){
		err = BIGLIB_ERR_FILENAME_TAKEN;
		return 0;
	}

	biglib_file* file = malloc(sizeof(biglib_file));
	file->name = malloc(strlen(name)+1);
	strcpy(file->name, name);
	file->type = BIGLIB_TYPE_BYTE_ARRAY;
	file->source = malloc(sizeof(biglib_source_byte_array));
	file->cursor = 0;
	biglib_source_byte_array* source = file->source;
	source->data = malloc(size);
	if (!source->data){
		free(file->source);
		free(file->name);
		free(file);
		err = BIGLIB_ERR_ALLOC_FAIL;
		return 1;
	}
	source->size = size;
	for (size_t i = 0; i < size; i++)
		source->data[i] = data[i];
	big->files = realloc(big->files, sizeof(void*)*(big->amount+1));
	big->files[big->amount++] = file;

	return 1;
}

int biglib_addStream(biglib_big* big, const char* name, FILE* stream){
	if (!big || !name || !stream){
		err = BIGLIB_ERR_ARGUMMENT_MISSING;
		return 0;
	}
	if (biglib_exists(big, name)){
		err = BIGLIB_ERR_FILENAME_TAKEN;
		return 0;
	}

	biglib_file* file = malloc(sizeof(biglib_file));
	file->name = malloc(strlen(name)+1);
	strcpy(file->name, name);
	file->type = BIGLIB_TYPE_STREAM;
	file->source = stream;
	file->cursor = 0;
	big->files = realloc(big->files, sizeof(void*)*(big->amount+1));
	big->files[big->amount++] = file;

	return 1;
}

int biglib_addStreamArea(biglib_big* big, const char* name, FILE* stream, unsigned long int location, size_t size){
	if (!big || !name || !stream){
		err = BIGLIB_ERR_ARGUMMENT_MISSING;
		return 0;
	}
	if (biglib_exists(big, name)){
		err = BIGLIB_ERR_FILENAME_TAKEN;
		return 0;
	}

	biglib_file* file = malloc(sizeof(biglib_file));
	file->name = malloc(strlen(name)+1);
	strcpy(file->name, name);
	file->type = BIGLIB_TYPE_STREAM_AREA;
	file->source = malloc(sizeof(biglib_source_stream_area));
	file->cursor = 0;
	biglib_source_stream_area* source = file->source;
	source->stream = stream;
	source->size = size;
	source->location = location;
	big->files = realloc(big->files, sizeof(void*)*(big->amount+1));
	big->files[big->amount++] = file;

	return 1;
}





int biglib_remove(biglib_big* big, const char* name){
	if (!big || !name){
		err = BIGLIB_ERR_ARGUMMENT_MISSING;
		return 0;
	}
	if (!biglib_exists(big, name)){
		err = BIGLIB_ERR_FILENAME_INVALID;
		return 0;
	}

	uint32_t pos = biglib_index(big, name);
	biglib_freeFile(big->files[pos]);
	big->amount--;
	for (uint32_t i = pos; i < big->amount; i++)
		big->files[i] = big->files[i+1];
	big->files = realloc(big->files, sizeof(void*)*big->amount);

	return 1;
}

int biglib_replaceFile(biglib_big* big, const char* name, const char* filename){
	if (!big || !name || !filename){
		err = BIGLIB_ERR_ARGUMMENT_MISSING;
		return 0;
	}
	if (!biglib_exists(big, name)){
		err = BIGLIB_ERR_FILENAME_INVALID;
		return 0;
	}

	biglib_remove(big, name);
	return biglib_addFile(big, name, filename);
}

int biglib_replaceData(biglib_big* big, const char* name, const unsigned char* data, size_t size){
	if (!big || !name || !data){
		err = BIGLIB_ERR_ARGUMMENT_MISSING;
		return 0;
	}
	if (!biglib_exists(big, name)){
		err = BIGLIB_ERR_FILENAME_INVALID;
		return 0;
	}

	biglib_remove(big, name);
	return biglib_addData(big, name, data, size);
}

int biglib_replaceStream(biglib_big* big, const char* name, FILE* stream){
	if (!big || !name || !stream){
		err = BIGLIB_ERR_ARGUMMENT_MISSING;
		return 0;
	}
	if (!biglib_exists(big, name)){
		err = BIGLIB_ERR_FILENAME_INVALID;
		return 0;
	}

	biglib_remove(big, name);
	return biglib_addStream(big, name, stream);
}

int biglib_replaceStreamArea(biglib_big* big, const char* name, FILE* stream, unsigned long int location, size_t size){
	if (!big || !name || !stream){
		err = BIGLIB_ERR_ARGUMMENT_MISSING;
		return 0;
	}
	if (!biglib_exists(big, name)){
		err = BIGLIB_ERR_FILENAME_INVALID;
		return 0;
	}

	biglib_remove(big, name);
	return biglib_replaceStreamArea(big, name, stream, size, location);
}





static int biglib_cmp_func(const void* a, const void* b){
	return strcmp((*((biglib_file**)(a)))->name, (*((biglib_file**)(b)))->name);
}

int biglib_sort(biglib_big* big){
	qsort(big->files, big->amount, sizeof(biglib_file*), biglib_cmp_func);
	return 1;
}

int biglib_exists(biglib_big* big, const char* name){
	if (!big || !name){
		err = BIGLIB_ERR_ARGUMMENT_MISSING;
		return 0;
	}
	for (uint32_t i = 0; i < big->amount; i++){
		if (!strcmp(big->files[i]->name, name))
			return 1;
	}
	return 0;
}

int biglib_index(biglib_big* big, const char* name){
	if (!big || !name){
		err = BIGLIB_ERR_ARGUMMENT_MISSING;
		return 0;
	}
	for (uint32_t i = 0; i < big->amount; i++){
		if (!strcmp(big->files[i]->name, name))
			return i;
	}
	return 0;
}





int biglib_freeFile(biglib_file* file){
	free(file->name);
	switch (file->type){
		case BIGLIB_TYPE_FILENAME:
		case BIGLIB_TYPE_STREAM_AREA:
		      free(file->source);
		      break;
		case BIGLIB_TYPE_BYTE_ARRAY:
		      free(((biglib_source_byte_array*)(file->source))->data);
		      free(file->source);
		      break;
	}
	free(file);

	err = 0;
	return 1;
}
int biglib_free(biglib_big* big){
	if (big->amount){
		for (uint32_t n = 0; n < big->amount; n++){
			biglib_freeFile(big->files[n]);
		}
		free(big->files);
	}
	if (big->stream)
		fclose(big->stream);
	free(big);

	err = 0;
	return 1;
}

int biglib_getError(char* log, int logLen){
	char* errStr;
	switch (err){
		case 0:
			errStr = "No error";
			break;
		case BIGLIB_ERR_NOT_SUPPORTED:
			errStr = "The function is not supported on this OS";
			break;
		case BIGLIB_ERR_INVALID_FILE:
			errStr = "The file format does not match csf file format";
			break;
		case BIGLIB_ERR_FAILED_TO_OPEN_FILE:
			errStr = "Failed to open file eather for read or write";
			break;
		case BIGLIB_ERR_FILENAME_TAKEN:
			errStr = "The file with the specified name already exists";
			break;
		case BIGLIB_ERR_FILENAME_INVALID:
			errStr = "The file with specified name could not be found";
			break;
		case BIGLIB_ERR_ARGUMMENT_MISSING:
			errStr = "One or more required argumments are missing";
			break;
		case BIGLIB_ERR_ALLOC_FAIL:
			errStr = "Failed to allocate memory";
			break;
	}
	strncpy(log, errStr, logLen);
	return err;
}
