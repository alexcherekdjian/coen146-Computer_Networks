 /**************************
 Alex Cherekdjian
 Thursday 5:15-8:00pm
 This program is the server. The server waits for a connection request(listen and accept). The client requests a connection 
 (connect) and then sends the name of file <output>(string + ‘\0’).The server receives the name of the file, opens the file, and waits for 
 data.The server receives the data sent to it by the client in chunks of 5 bytes and writes the data to file <output>. It then closes the
 connection.
 ***************************/

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#define char_buffer 5

int main (int, char *[]); 


/*********************
 * main
 *********************/
int main (int argc, char *argv[])
{
	if (argc != 2){
		printf("wrong number of arguments"); // if not enough arguments given exit program
		return -1;
	}
	
	int portNumber = atoi(argv[1]);	// convert the port number given into integers
	
	int n; 
	char *p; 
	int listenfd = 0, connfd = 0;
	struct sockaddr_in serv_addr; // initializing struct for socket
	char buff[char_buffer]; // initializing character buffer

	memset (&serv_addr, '0', sizeof (serv_addr)); // setting everything in buff to zero
	memset (buff, '0', sizeof (buff)); // setting everything in the struct to zero

	serv_addr.sin_family = AF_INET; // setting the family value in struct to AF_INET constant
	serv_addr.sin_addr.s_addr = htonl (INADDR_ANY); // usinging INADDR_ANY so the socket is not binded to a single IP
	serv_addr.sin_port = htons(portNumber); // setting the port number and converting it into network byte information using htons

	// create socket, bind, and listen
	listenfd = socket (AF_INET, SOCK_STREAM, 0); // create a default (0) TCP socket in order to talk to other devices with AF_INET addresses
	bind (listenfd, (struct sockaddr*)&serv_addr, sizeof (serv_addr)); // specifies an address to socket given by serv_addr with a length of serv_addr
	listen (listenfd, 10); // listen at socket denoted at listenfd for a backlog queue of 10
	

	int connfd1 = 0; 
	FILE *dst; // creating file pointer
	char fileBuffer[1025]; // creating file buffer

	if((connfd1 = accept(listenfd, (struct sockaddr*)NULL, NULL)) == -1){ // first accept to get file name
		printf("error in first accept");
	} 

	// reading file name
	if(read(connfd1, fileBuffer, sizeof(fileBuffer)) > 0){
		dst = fopen(fileBuffer, "wb"); // if successful then open file
	}else{
		printf("error in reading file name\n");
		return 1;
	}

	while ((n = read(connfd1,buff, sizeof(buff))) > 0){
		fwrite(buff,sizeof(char),n,dst); // as long as successful read, write to file
	}
		
	fclose(dst); // close file
	close (connfd); // closes the connection
return 0;
}

