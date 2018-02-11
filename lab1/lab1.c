// Alex Cherekdjian

#include <stdio.h>
#include <string.h>
#define char_buffer 10

int main(int argc, char *argv[]){

	if(argc != 3){	// ensuring proper arguments passed
	return -1;
	}

	FILE *src, *dst;	// creating file pointers
	char word[char_buffer]; // creating array with size of buffer to store words

	src = fopen(argv[1], "rb"); // opening corresponding files
	dst = fopen(argv[2], "wb");
	int size = 0;	// initializing size of amount of bytes to copy


	while((size = fread(word,1,char_buffer,src)) != 0){ // copying from src to dst
		fwrite(word,size,1,dst);
	}

	fclose(src); // closing files and releasing pointers
	fclose(dst);

return 0;
}



