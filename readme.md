# biglib

This is a library to view, edit and write big files.
For commands view the header file.

## Read example

```C
#include <stdio.h>
#include <stdlib.h>
#include <biglib.h>

int main(){
	biglib_big* file = biglib_readFile("file.big");		// Opens big file


	size_t size = biglib_getSize(file, "hello.txt");	// Get size of "hello.txt"
	char* data = malloc(size+1);				// Allocate space for "hello.txt" content

	biglib_fread(file, "hello.txt", data, 1, size);		// Read from "hello.txt" to data buffer amount equal to size
	data[size] = '\0';					// Add null terminator


	puts(data);						// Print data


	free(data);						// Free data buffer
	biglib_free(file);					// Close big file
}
```
This will print content of the file "hello.txt" in "file.big".

## Write example

```C
#include <stdio.h>
#include <stdlib.h>
#include <biglib.h>

int main(){
	biglib_big* file = biglib_create();			// Create new big file


	biglib_addData(file, "hello.txt", "Hello, world!", 13);	// Add file called "hello.txt" with data "Hello, world!" size of 13 in it
	biglib_writeFile(file, "file.big");			// Write big file to a real file


	biglib_free(file);					// Close big file
}
```
this will create "file.big" with "file.txt" in it containing "Hello, world!".
