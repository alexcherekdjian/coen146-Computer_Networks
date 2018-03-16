/**********************
 * Alex Cherekdjian
 * COEN 146, UDP Server
 * Includes Timer, fake CheckSum, skip & fake ACK
 **********************/

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#define BUFFER_SIZE 10
#define error 50 // percentage of packets not sent

typedef struct{
	int seq_ack;
	int length;
	int checksum;

} HEADER;

typedef struct{
	HEADER header;
	char data[BUFFER_SIZE];
} PACKET;

//int calcCheckSum(void *point, int len);
int compute_checksum(PACKET pkt, int err);
bool fakeSendACK(int seq);
void prepareNAK(int seq);
void prepareACK(int seq);

int sendOrSkip = 0;
PACKET ACKed;
PACKET NAKd;

int main (int argc, char *argv[])
{
	int sock, nBytes;
	char buffer[1024];
	struct sockaddr_in serverAddr, clientAddr;
	struct sockaddr_storage serverStorage;
	socklen_t addr_size, client_addr_size;
	int i;

    if (argc != 2){
        printf ("need the port number\n");
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
		printf ("socket error\n");
		return 1;
	}

	// bind
	if (bind (sock, (struct sockaddr *)&serverAddr, sizeof (serverAddr)) != 0){
		printf ("bind error\n");
		return 1;
	}

	int connfd1 = 0;
	if((connfd1 = accept(sock, (struct sockaddr*)NULL, NULL)) != -1){ 
		printf("error in first accept"); // accept connection, if not then error
	} 
	 
	FILE *dst; // creating file pointer
	char fileBuffer[1025]; // creating file buffer
	int bytesWritten = 0;
	int testSum = 0;
	int recievedCheckSum = 0;
	bool fakeSent = false; 


	while(1){
		PACKET recievedPacket;
		recvfrom(sock, &recievedPacket, sizeof(recievedPacket), 0, (struct sockaddr *)&serverStorage, &addr_size); // recieve next packet
		printf("packet recieved: sequence %d\n", recievedPacket.header.seq_ack); // print message to user
		
		// calc check sum
		testSum = 0;
		recievedCheckSum = recievedPacket.header.checksum;
		recievedPacket.header.checksum = 0;
		testSum = compute_checksum(recievedPacket, error);

		
		if(testSum == recievedCheckSum){
			// if correct, attempt to send a fake
			fakeSent = fakeSendACK(recievedPacket.header.seq_ack);

			if(fakeSent == false){
				for (i = 0; i < recievedPacket.header.length; ++i){
					fileBuffer[(bytesWritten + i)] = recievedPacket.data[i]; // if no fake sent, read data in
					printf("recieved: %c \n", recievedPacket.data[i]);

				}
				bytesWritten += recievedPacket.header.length; // increment buffer
				prepareACK(recievedPacket.header.seq_ack); // prepare real ACK
			}

			sendto(sock, &ACKed, sizeof(ACKed), 0, (struct sockaddr*)&serverStorage, addr_size); // send
			break; // can break assuming file name < 9 chars

		}else{
			prepareNAK(recievedPacket.header.seq_ack); // prepare real NAK
			sendto(sock, &NAKd, sizeof(NAKd), 0,(struct sockaddr*)&serverStorage, addr_size); // send
		}
	}

	// file name recieved open and continue to file contents
	dst = fopen(fileBuffer, "wb"); // if successful then open file
	int finalSequence; 
  
	while(1){
		PACKET recievedData;
		recvfrom(sock, &recievedData, sizeof(recievedData), 0, (struct sockaddr *)&serverStorage, &addr_size); // recieve data packet
		
		if(recievedData.header.length == 0){
			printf("last packet recieved: sequence terminated\n");
			finalSequence = recievedData.header.seq_ack; // if final packet (len 0) store seq ack and break
			break; // last packet recieved
		}

		printf("packet recieved: sequence %d\n", recievedData.header.seq_ack);
		
		// calc check sum
		testSum = 0;
		recievedCheckSum = recievedData.header.checksum;
		recievedData.header.checksum = 0;
		testSum = compute_checksum(recievedData, error);
		
		if(testSum == recievedCheckSum){
			// if correct, attempt to send a fake				
			fakeSent = fakeSendACK(recievedData.header.seq_ack);

			if(fakeSent == false){
				for (i = 0; i < recievedData.header.length; ++i){
					buffer[(i)] = recievedData.data[i]; // if fake not sent write to buffer
					printf("data recieved: %c\n",recievedData.data[i]);
				}

				fwrite(buffer,sizeof(char),recievedData.header.length,dst); // as long as successful write to file
				prepareACK(recievedData.header.seq_ack); // make real ACK
			}

			sendto(sock, &ACKed, sizeof(ACKed), 0, (struct sockaddr*)&serverStorage, addr_size); // send

		}else{
			prepareNAK(recievedData.header.seq_ack); // make real NAK
			sendto(sock, &NAKd, sizeof(NAKd), 0, (struct sockaddr*)&serverStorage, addr_size); // send
		}
	}

	 // init final ending packet with len 0 and finalSequence number
	ACKed.header.length = 0; 
	ACKed.header.seq_ack = finalSequence;
	sendto(sock, &ACKed, sizeof(ACKed), 0, (struct sockaddr*)&serverStorage, addr_size); // send
	printf("Final sequence ACKed\n");
	
	fclose(dst); // close file
	close(connfd1); // closes the connection

	return 0;
}

int compute_checksum(PACKET pkt, int err){
    if( (rand() % 100) < err) {
        printf("Incorrect Calculation\n"); // sends a wrong checksum
        return 0;
    } else { 

	    // Return valid checksum  
	    int sum = 0;
	    char *buffer = (char *)pkt.data; // init char pointer
	    sum = *buffer++;
	    int i;

	    for(i=0; i<pkt.header.length; i++) {
	        if(buffer != '\0') {
	            sum = sum ^ *buffer; // XOR checksum with data
	            buffer++;
	        }
	    }
	return sum;
    }
}

bool fakeSendACK(int seq){
	if( (rand() % 100) < error){
		if(sendOrSkip == 0){
			// send a wrong ack
			ACKed.header.length = 0;
			if(seq == 0){
				ACKed.header.seq_ack = 1;
			}else{
				ACKed.header.seq_ack = 0;
			}
			printf("packet correct: SENDING WRONG ACKing sequence %d\n", seq);
			sendOrSkip = 1; // rotate between skip and send wrong ack
			return true;
		}else{
			// skip the ack
			sendOrSkip = 0; // rotate between skip and send wrong ack
			printf("packet correct: SKIPPING ACK %d\n", seq);
			return true;
		}			
	}
	return false;	
}

void prepareACK(int seq){
	ACKed.header.length = 0; // init length
	ACKed.header.seq_ack = seq; // init sequence
	printf("packet correct: ACKing sequence %d\n", seq);
}

void prepareNAK(int seq){
	NAKd.header.length = 0; // init length
	if(seq == 1){
		NAKd.header.seq_ack = 0; // opposite seq
	}else{
		NAKd.header.seq_ack = 1;
	}
	printf("packet wrong: NAKing sequence %d\n", seq);
}

