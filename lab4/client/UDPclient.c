/**********************
 * Alex Cherekdjian
 * COEN 146, UDP Client
 * Includes Timer & fake CheckSum
 **********************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>

#define BUFFER_SIZE 10

typedef struct{
	int seq_ack;
	int length;
	int checksum;
} HEADER;

typedef struct{
	HEADER header;
	char data[BUFFER_SIZE];
} PACKET;

int compute_checksum(PACKET pkt, int err);

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

	// opening file to send
	FILE *src;
	src = fopen(argv[3], "rb"); 

	int i = 0;
	int sequence = 1; // init sequence to 1
	int fileSize = sizeof(argv[4]); // get fileSize of dest file name
	int loopCounter = fileSize; // init loopCounter
	int bytesWritten = 0; // init bytes written

	struct timeval tv;// timer
	int rv; // select returned value
	
	// set it up, in the beginning of the function
	fd_set readfds;
	fcntl(sock, F_SETFL, O_NONBLOCK);
	
	// file name works for names < 9 characters
	while(loopCounter > 0){
		PACKET fileName;
		
		if(fileSize > BUFFER_SIZE)
			loopCounter = BUFFER_SIZE; // if larger than buffer bytes, only send buffer size
		
		for (i = 0; i < loopCounter; ++i){
			fileName.data[i] = argv[4][bytesWritten+i]; // read from filename into packet
		}

		fileName.header.length = strlen(fileName.data)+1; // init length
		fileName.header.seq_ack = sequence; // init sequence number
		fileName.header.checksum = 0; // init checksum to 0
		bytesWritten += fileName.header.length; // increment buffer

		fileName.header.checksum = compute_checksum(fileName, 0);// assigning actual checksum 
		
		while(1){
			FD_ZERO (&readfds);
			FD_SET (sock, &readfds);

			// set the timer
			tv.tv_sec = 10;
			tv.tv_usec = 0;

			printf("Sending packet %d\n", sequence);
			sendto(sock, &fileName, sizeof(fileName), 0, (struct sockaddr*)&serverAddr, addr_size); // send packet
			rv = select(sock+1,&readfds,NULL,NULL,&tv); // check timer

			PACKET ackOrNak; // create recieving packet
			recvfrom(sock,&ackOrNak,sizeof(ackOrNak),0,NULL, NULL); // wait to recieve from packet
			
			if(rv == 0){
				printf("TIMER EXPIRED: resending sequence %d\n", sequence); // timer expired so resend packet
				continue;
			}else if(rv == 1){
				if(ackOrNak.header.seq_ack == sequence){
					printf("packet recieved ACK %d\n", ackOrNak.header.seq_ack);
					if(sequence == 1){
						sequence = 0; // if correct packet ack recieved then change sequence
					}else{
						sequence = 1;
					}
					break; // break loop and send next packet
				}
			}	
			printf("packet not recieved trying %d again\n", sequence);
			// will resend if wrong ack or nak recieved
		}
		loopCounter = fileSize - loopCounter; // if packet sent decrement char buffer size counter
	}

	while(!feof(src)){
		PACKET fileData;

		int bytes = fread(fileData.data, sizeof(char), BUFFER_SIZE, src); // store number of bytes sent in packet
		
		fileData.header.length = bytes; // store number of bytes read in data length
		fileData.header.seq_ack = sequence; // init sequence number
		fileData.header.checksum = 0; // init checksum to 0
		fileData.header.checksum = compute_checksum(fileData, 0); // assigning actual checksum
 
		while(1){
			FD_ZERO (&readfds);
			FD_SET (sock, &readfds);

			// set the timer
			tv.tv_sec = 10;
			tv.tv_usec = 0;

			printf("Sending packet %d\n", sequence);
			sendto(sock, &fileData, sizeof(fileData), 0, (struct sockaddr*)&serverAddr, addr_size); // send packet
			rv = select(sock+1,&readfds,NULL,NULL,&tv); // check timer

			PACKET ackOrNak; // create recieving packet
			recvfrom(sock,&ackOrNak,sizeof(ackOrNak),0,NULL, NULL); // wait to recieve from packet
			
			if(rv == 0){
				printf("TIMER EXPIRED: resending sequence %d\n", sequence); // timer expired so resend packet
				continue;
			}else if(rv == 1){
				printf("packet recieved ACK %d\n", ackOrNak.header.seq_ack);
				if(ackOrNak.header.seq_ack == sequence){
					if(sequence == 1){
						sequence = 0; // if correct packet ack recieved then change sequence
					}else{
						sequence = 1;
					}
					break; // break loop and send next packet
				}
			}
		}
	}

	for(i = 0; i != 3; ++i){
		FD_ZERO (&readfds);
		FD_SET (sock, &readfds);

		// set the timer
		tv.tv_sec = 10;
		tv.tv_usec = 0;

		PACKET doneSendingFiles;
		doneSendingFiles.header.length = 0; // send final ending packet with length 0 max of 3 times
		sendto(sock, &doneSendingFiles, sizeof(doneSendingFiles), 0, (struct sockaddr*)&serverAddr, addr_size); // tell server we are done with sending files
		
		rv = select(sock+1,&readfds,NULL,NULL,&tv); // check the timer
		PACKET ackOrNak; // create recieving packet
		recvfrom(sock,&ackOrNak,sizeof(ackOrNak),0,NULL, NULL); // wait to recieve from packet

		if(rv == 0){
			printf("TIMER EXPIRED: resending FINAL sequence %d \n", sequence); // timer expired so resend packet
			continue;
		}else if(rv == 1){
			if(ackOrNak.header.seq_ack == sequence){
				printf("End packet recieved ACK %d \n", sequence); // last ack recieved
				break;
			}
		}
	}

	fclose(src); // close the file
	close(sock); // closes the connection 

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
