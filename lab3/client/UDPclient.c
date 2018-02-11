/**********************
 * Alex Cherekdjian
 * COEN 146 - Networks
 * February, 8 2018
 * Thursday 5:15 PM
 * UDP Client
 **********************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#define BUFFER_SIZE 10 // buffer size

typedef struct{
	int seq_ack;
	int length;
	int checksum;
} HEADER;

typedef struct{
	HEADER header;
	char data[BUFFER_SIZE];
} PACKET;

int calcCheckSum(void *point, int len);

int main (int argc, char *argv[])
{
	int sock, portNum, nBytes;
	struct sockaddr_in serverAddr;
	socklen_t addr_size;

	if (argc != 5){
		printf ("The syntax is as follows: <PORT#> <IP> <SOURCE> <DESTINATION> \n");
		return 1;
	}

	// configure address
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons (atoi (argv[1]));
	inet_pton(AF_INET, argv[2], &serverAddr.sin_addr.s_addr);

	memset (serverAddr.sin_zero, '\0', sizeof (serverAddr.sin_zero));  
	addr_size = sizeof serverAddr;

	/*Create UDP socket*/
	sock = socket(PF_INET, SOCK_DGRAM, 0);

	FILE *src; // creating file pointer
	src = fopen(argv[3], "rb"); // opening file to send
	int i = 0; // initializing loop elements
	int sequence = 0; // initializing packet sequence at 0
	int fileSize = sizeof(argv[4]); // assigning fileSize of arg4
	int loopCounter = fileSize; // assigning to loopCounter for clarity
	int bytesWritten = 0; // initializing bytesWritten 
	bool finished = false; // initializing loop finished variable

	printf("Starting file name tranfer\n");
	while(finished != true){
		while(loopCounter > 0){
			PACKET fileName; // create fileName packet
			if(loopCounter < BUFFER_SIZE){
				finished = true; // signifies last packet and exit after sent
			}

			if(fileSize > BUFFER_SIZE)
				loopCounter = BUFFER_SIZE; // if larger than buffer bytes, only send buffer size

			printf("Copying data . . .\n");
			for (i = 0; i < loopCounter; ++i){
				fileName.data[i] = argv[4][bytesWritten+i]; // copy contents of arg4 into file data
			}

			printf("Preparing packet . . .\n");
			fileName.header.length = strlen(fileName.data)+1; // init length
			fileName.header.seq_ack = sequence; // init sequence number
			fileName.header.checksum = 0; // init checksum to 0
			bytesWritten += fileName.header.length; // add # of bytes written
			fileName.header.checksum = calcCheckSum(&fileName, fileName.header.length);// assigning actual checksum 

			while(1){
				printf("Sending packet sequence: %d \n", sequence);
				sendto(sock, &fileName, sizeof(fileName), 0, (struct sockaddr*)&serverAddr, addr_size); // send packet
				PACKET ackOrNak; // create recieving packet
				recvfrom(sock,&ackOrNak,sizeof(ackOrNak),0,NULL, NULL); // wait to recieve from packet
				if(ackOrNak.header.seq_ack == sequence){
					printf("ACK packet recieved sequence: %d \n", ackOrNak.header.seq_ack);
					if(sequence == 1){
						sequence = 0; // if correct packet ack recieved then change sequence
					}else{
						sequence = 1;
					}
					break; // break loop and send next packet
				}
				printf("NAK packet recieved seqeunce: %d \nResending. . .\n", sequence); // will resend if wrong ack or nak recieved
			}
			loopCounter = fileSize - loopCounter; // if packet sent decrement char buffer size counter
		}
	}
	printf("Final file name packet sent \n");

	// ending file name send
	printf("Terminating file name transfer\n");
	PACKET doneSendingFileName; // sending final packet of length 0 signifying file name end
	doneSendingFileName.header.length = 0;
	sendto(sock, &doneSendingFileName, sizeof(doneSendingFileName), 0, (struct sockaddr*)&serverAddr, addr_size); // tell server we are done with sending fileName
	printf("Terminating Successful\n");
	
	i = 0; // reinitializing i variable 
	int bytes = 0; // initializing bytes read from file 

	printf("Starting file data tranfer\n");
	while(!feof(src)){
		PACKET fileData; // creating packet to send file data

		printf("Copying data . . .\n");
		bytes = fread(fileData.data, sizeof(char), BUFFER_SIZE, src); // store number of bytes sent in packet

		printf("Preparing packet . . .\n");
		fileData.header.length = bytes; // init length
		fileData.header.seq_ack = sequence; // init sequence number
		fileData.header.checksum = 0; // init checksum to 0
		fileData.header.checksum = calcCheckSum(&fileData, fileData.header.length); // assigning actual checksum

		while(1){
			printf("Sending packet sequence: %d \n", sequence);
			sendto(sock, &fileData, sizeof(fileData), 0, (struct sockaddr*)&serverAddr, addr_size); // send packet
			PACKET ackOrNak; // create recieving packet
			recvfrom(sock,&ackOrNak,sizeof(ackOrNak),0,NULL, NULL); // wait to recieve from packet
			if(ackOrNak.header.seq_ack == sequence){
				printf("ACK packet recieved sequence: %d \n", ackOrNak.header.seq_ack);
				if(sequence == 1){
					sequence = 0; // if correct packet ack recieved then change sequence
				}else{
					sequence = 1;
				}
				break; // break loop and send next packet
			}
			printf("NAK packet recieved seqeunce: %d \nResending. . .\n", sequence); // will resend if wrong ack or nak recieved
		}
	}
	printf("Final file data package sent\n");

	// ending file data send
	printf("Terminating file name transfer\n");
	PACKET doneSendingFiles; // sending final packet of length 0 signifying file name end
	doneSendingFiles.header.length = 0;
	sendto(sock, &doneSendingFiles, sizeof(doneSendingFiles), 0, (struct sockaddr*)&serverAddr, addr_size); // tell server we are done with sending files
	printf("Terminating Successful\n");

	fclose(src); // close the file
	close(sock); // closes the connection 
	printf("Connections closed succesfully \n");

	return 0;
}

int calcCheckSum(void *point, int len){
	unsigned char* pack = point; // init pointer
	int temp = 0; // init temp to 0
	int count; // init count of loop below

	for (count = 0; count < sizeof(HEADER) + len; count++){
		temp ^= *(++pack); // calculating actual checksum using XOR of header and data
	}
	return temp;
}
