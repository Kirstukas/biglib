# biglib

This is a library to view, edit and write big files.
For commands view the header file.

## Read example

```C
#include <stdio.h>
#include <stdlib.h>
#include <biglib.h>

int main(){
	biglib_big* file = biglib_readFile("file.big");


	size_t size;
	unsigned char* data = biglib_get(file, "hello.txt", &size);


	fwrite(data, 1, size, stdout);


	free(data);
	biglib_free(file);
}
```
This will print content of the file "hello.txt" in "file.big".

## Write example

```C
#include <stdio.h>
#include <stdlib.h>
#include <biglib.h>

int main(){
	biglib_big* file = biglib_create();


	biglib_addData(file, "hello.txt", "Hello, world!", 13);

	biglib_writeFile(file, "file.big");


	biglib_free(file);
}
```
this will create "file.big" with "file.txt" in it containing "Hello, world!".
