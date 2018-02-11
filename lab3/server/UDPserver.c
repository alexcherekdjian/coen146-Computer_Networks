/**********************
 * Alex Cherekdjian
 * COEN 146 - Networks
 * February, 8 2018
 * Thursday 5:15 PM
 * UDP Server
 **********************/

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
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
	int sock, nBytes;
	char buffer[1024];
	struct sockaddr_in serverAddr, clientAddr;
	struct sockaddr_storage serverStorage;
	socklen_t addr_size, client_addr_size;
	int i;

    if (argc != 2){
        printf ("Need the port number\n");
        return 1;
    }

	// init 
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons ((short)atoi (argv[1]));
	serverAddr.sin_addr.s_addr = htonl (INADDR_ANY);
	memset ((char *)serverAddr.sin_zero, '\0', sizeof (serverAddr.sin_zero));  
	addr_size = sizeof (serverStorage);

	// create socket
	if ((sock = socket (AF_INET, SOCK_DGRAM, 0)) < 0){
		printf ("Socket error\n");
		return 1;
	}

	// bind
	if (bind (sock, (struct sockaddr *)&serverAddr, sizeof (serverAddr)) != 0){
		printf ("Bind error\n");
		return 1;
	}

	int connfd1 = 0;
	if((connfd1 = accept(sock, (struct sockaddr*)NULL, NULL)) != -1){ 
		printf("Error in first accept"); // accept connection, if not then error
	} 
	 
	FILE *dst; // creating file pointer
	char fileBuffer[1025]; // creating file buffer
	int bytesWritten = 0; // init bytes written
	int testSum = 0; // init test sum
	int recievedCheckSum = 0; // init var to store check sum gained


	while(1){
		PACKET recievedPacket; // initializing first packet
		recvfrom(sock, &recievedPacket, sizeof(recievedPacket), 0, (struct sockaddr *)&serverStorage, &addr_size); // recieve package from client

		if(recievedPacket.header.length == 0){
			break; // last packet recieved
			printf("Last file name packet recieved sequence terminated\n");
		}
		printf("File name packet recieved\n");
		printf("Packet recieved sequence: %d\n", recievedPacket.header.seq_ack);
		// calc check sum
		testSum = 0;
		recievedCheckSum = recievedPacket.header.checksum; // storing check sum in variable
		recievedPacket.header.checksum = 0; // init package checksum 

		testSum = calcCheckSum(&recievedPacket, recievedPacket.header.length); // calculating check sum

		if(testSum == recievedCheckSum){
			for (i = 0; i < recievedPacket.header.length; ++i){
				fileBuffer[(bytesWritten + i)] = recievedPacket.data[i]; // if correct, copy data into buffer
			}
			bytesWritten += recievedPacket.header.length; // storing how much stored in buffer

			PACKET ACKed; // creating ack packet
			ACKed.header.length = 0; // init length to 0
			ACKed.header.seq_ack = recievedPacket.header.seq_ack; // returning same sequence number of packet
			printf("Correct packet recieved: ACKing sequence %d\n", recievedPacket.header.seq_ack);
			sendto(sock, &ACKed, sizeof(ACKed), 0, (struct sockaddr*)&serverStorage, addr_size); // send ack
		}else{
			PACKET NAKd; // creating nak packet
			NAKd.header.length = 0; // init length to 0
			if(recievedPacket.header.seq_ack == 1){
				NAKd.header.seq_ack = 0; // setting sequence to opposite of what package came in
			}else{
				NAKd.header.seq_ack = 1;
			}

			printf("Incorrect packet recieved: NAKing sequence %d\n", recievedPacket.header.seq_ack);
			sendto(sock, &NAKd, sizeof(NAKd), 0,(struct sockaddr*)&serverStorage, addr_size); // send nak
		}
	}
	// file name recieved open and continue to file contents
	dst = fopen(fileBuffer, "wb"); // if successful then open file
  
	while(1){
		PACKET recievedData; // create recieved packet file
		recvfrom(sock, &recievedData, sizeof(recievedData), 0, (struct sockaddr *)&serverStorage, &addr_size); // recieving packet
		printf("File data packet recieved\n");

		if(recievedData.header.length == 0){
			printf("Last file data packet recieved: sequence terminated\n");
			break; // last packet recieved
		}

		printf("Packet recieved: sequence %d\n", recievedData.header.seq_ack);
		// calc check sum
		testSum = 0;
		recievedCheckSum = recievedData.header.checksum;
		recievedData.header.checksum = 0;

		testSum = calcCheckSum(&recievedData, recievedData.header.length); // calculating check sum
		
		if(testSum == recievedCheckSum){
			for (i = 0; i < recievedData.header.length; ++i){
				buffer[(i)] = recievedData.data[i];
			}

			fwrite(buffer,sizeof(char),recievedData.header.length,dst); // as long as successful read, write to file

			PACKET ACKed; // creating ack packet
			ACKed.header.length = 0; // init length to 0
			ACKed.header.seq_ack = recievedData.header.seq_ack; // returning same sequence number of packet
			printf("Correct packet recieved: ACKing sequence %d\n", recievedData.header.seq_ack);
			sendto(sock, &ACKed, sizeof(ACKed), 0, (struct sockaddr*)&serverStorage, addr_size); // send ack
		}else{
			PACKET NAKd; // creating nak packet
			NAKd.header.length = 0; // init length to 0
			if(recievedData.header.seq_ack == 1){
				NAKd.header.seq_ack = 0; // setting sequence to opposite of what came in
			}else{
				NAKd.header.seq_ack = 1;
			}

			printf("Incorrect packet recieved: NAKing sequence %d\n", recievedData.header.seq_ack);
			sendto(sock, &NAKd, sizeof(NAKd), 0, (struct sockaddr*)&serverStorage, addr_size); // send nak
		}
	}
	
	fclose(dst); // close file
	close(connfd1); // closes the connection
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
