 /**************************
 Alex Cherekdjian
 Thursday 5:15-8:00pm
 This program is the client. The client accepts 4 arguments: the name of the two files(<input> and <output>), and the IP address and the port 
 number of the server.The client opens file <input>. Then it reads file <input> and sends the data in chunks of 10 bytes to the server. It then 
 closes the connection.
 **************************/

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h> 
#define client_buffer 10

int main (int, char *[]);


/********************
 * main // argument order = port number, IP of server, source, destination
 ********************/
int main (int argc, char *argv[])
{
	int i;
	int sockfd = 0, n = 0;
	char buff[client_buffer]; // initializing character buffer
	char *p;
	struct sockaddr_in serv_addr; // initializing struct for socket

	if (argc != 5){ // ensuring correct number of arguments passed in excecution
		printf ("Usage: %s <ip of server> \n",argv[0]);
		return 1;
	} 

	memset (buff, '0', sizeof (buff)); // setting everything in buff to zero
	memset (&serv_addr, '0', sizeof (serv_addr)); // setting everything in the struct to zero

	if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) < 0){ // specifies to create a default (0) TCP socket to talk to other devices with AF_INET addresses
		printf ("Error : Could not create socket \n"); // if return not 0, then unsuccessful
		return 1;
	} 

	serv_addr.sin_family = AF_INET; // setting the family value in struct to AF_INET constant
	serv_addr.sin_port = htons (atoi(argv[1])); // setting the port number and converting it into network byte information using htons

	if (inet_pton (AF_INET, argv[2], &serv_addr.sin_addr) <= 0){ 
		printf ("inet_pton error occured\n"); // checking to see if the package created is correctly ordered and made
		return 1;
	} 

	if (connect (sockfd, (struct sockaddr *)&serv_addr, sizeof (serv_addr)) < 0){
		printf ("Error : Connect Failed \n"); // connects our socket to the server address given the size of address
		return 1;
	} 

	FILE *src;
	src = fopen(argv[3], "rb"); // opening file to send
	
	int error;
	if((error = send(sockfd, argv[4], sizeof(argv[4]), 0)) == -1){
	printf("Error in sending dst file name"); // ensure file is sent through socket
	}

	int bytes;
	while(!feof(src)){
		bytes = fread(buff, sizeof(char), client_buffer, src); // as long as not EOF read bytes into buffer and write to socket
		write(sockfd, buff, bytes);
	}

	fclose(src); // close the file
	close (sockfd); // closes the connection 

return 0;
}
