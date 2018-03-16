/**********************
 * Alex Cherekdjian
 * COEN 146 - Networks
 * March, 15 2018
 * Thursday 5:15 PM
 * Lab 5 - Routing
 **********************/

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>      
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

pthread_mutex_t cost_lock;

typedef struct {
	char name[50];
	char ip[50];
	int port;

} machine;

void dijkstra(int cost_matrix[4][4], int source);
void print_lowestCost();
void print_cost_table();
void* thread_3();
void* thread_1();

// init global variables
int my_router_id;
int my_port_number;
int nodes;
int cost[4][4];
int least_cost[4];
int i,j; // init loop variables


// init network global variables
int sock, nBytes;
struct sockaddr_in serverAddr;
socklen_t addr_size;

int main (int argc, char *argv[]){

    if (argc != 5){
        printf("Wrong number of inputs\n"); // print error if wrong number of arguments
        return 1;
    }

   	my_router_id = (short)atoi(argv[1]); // convert router info from char* to short
   	nodes = (int)atoi(argv[2]); // convert node info into int

   	if (nodes != 4){
   		printf("Nodes needs to be 4\n"); // ensure correct number of nodes entered
		return 1;
	}

    // create filename pointers
   	char* cost_file = argv[3];
   	char* host_file = argv[4];
    
    // init routers array
   	machine routers[nodes]; 

    // init file pointers and open files
   	FILE *fp1;
   	FILE *fp2;
    fp1 = fopen(host_file, "r");
    fp2 = fopen(cost_file, "r");

    for (i = 0; i < nodes; ++i){
		fscanf(fp1, "%s %s %d", routers[i].name, routers[i].ip, &routers[i].port); // scan from file into router info
		printf("Machine Recieved: %s %s %d \n", routers[i].name, routers[i].ip, routers[i].port); // report to user what read in
		if(my_router_id == i){
			my_port_number = routers[i].port; // if router id matches this machines, store port number
		}

    }

	for(i = 0; i < nodes; ++i){
        for( j = 0; j < nodes; ++j){
            fscanf(fp2, "%d", &cost[i][j]); // scan from file costs
            printf("Cost Recieved: %d \n", cost[i][j]); // report to user the value read in
        }
    }
	printf("This Router's IP, ID, INDEX: %s %s %d\n\n", routers[my_router_id].ip, routers[my_router_id].name, my_router_id); // report this machines information

    // initialize socket information
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons ((short)my_port_number);
	serverAddr.sin_addr.s_addr = htonl (INADDR_ANY);
	memset ((char *)serverAddr.sin_zero, '\0', sizeof (serverAddr.sin_zero));  
	addr_size = sizeof (serverAddr);

	// create socket
	if ((sock = socket (AF_INET, SOCK_DGRAM, 0)) < 0){
		printf ("Socket Error\n");
        exit(1);
	}

	// bind to socket
	if (bind (sock, (struct sockaddr *)&serverAddr, sizeof (serverAddr)) != 0){
		printf ("Bind Error\n");
        exit(1);
	}

    // init threads 1 and 3 (2 is main)
	pthread_t thread3;
	pthread_t thread1;

    // create threads
	pthread_mutex_init(&cost_lock, NULL);
	pthread_create(&thread1,NULL,thread_1, NULL);
	pthread_create(&thread3,NULL,thread_3, NULL);

    // Thread 2 Implementation
    for(i = 0; i < 2; ++i){
    	int neighbor_id_rec, new_cost_rec; // init recieved id and cost
    	printf("Enter a new neighbor and new cost: "); // prompt user
    	scanf("%d %d", &neighbor_id_rec, &new_cost_rec); // scan values into respective vars

	    // update costs table with new info
	    pthread_mutex_lock(&cost_lock);
	    cost[my_router_id][neighbor_id_rec] = new_cost_rec;
	    cost[neighbor_id_rec][my_router_id] = new_cost_rec;
        print_cost_table(); // print out cost table
	    pthread_mutex_unlock(&cost_lock);

	    for (j = 0; j < nodes; ++j){
	    	if(j == my_router_id)
	    		continue; // if router id is ours, do not send to ourselves

	        // configure the ip and port number for machine sending to
            serverAddr.sin_port = htons (routers[j].port);
            inet_pton(AF_INET, routers[j].ip, &serverAddr.sin_addr.s_addr);

			printf("Sending Update to %s:%d\n", routers[j].ip, routers[j].port); // report what we are sending

	        int answer[3] = {my_router_id, neighbor_id_rec, new_cost_rec}; // store all values in an array
            sendto(sock, answer, sizeof(answer), 0, (struct sockaddr *)&serverAddr, addr_size); // send over network
	 	}
		sleep(10); // sleep for 10 seconds
    }
    sleep(30); // sleep for 30 seconds
	
    // close all files and clean up threads
    fclose(fp1);
    fclose(fp2);
    pthread_mutex_destroy(&cost_lock);
    pthread_exit(NULL);

	return 0;
}

void* thread_1(){ 

	while(1){
        // init buffer and receiving variables
		int buffer[3];
	    int router_id_rec, neighbor_id_rec, new_cost_rec;

	    recvfrom(sock, buffer, sizeof(buffer), 0, NULL, NULL); // recieve data from neighbors

	    // store data in respective variables
	    router_id_rec = buffer[0];
	    neighbor_id_rec = buffer[1];
	    new_cost_rec = buffer[2];

	    printf("Recieved// R_ID: %d N_ID: %d NEW_COST: %d\n", router_id_rec, neighbor_id_rec, new_cost_rec); // report what has been recieved

	    // update costs table with new info
	    pthread_mutex_lock(&cost_lock);
	    cost[router_id_rec][neighbor_id_rec] = new_cost_rec;
	    cost[neighbor_id_rec][router_id_rec] = new_cost_rec;
        print_cost_table(); // print out cost table
	    pthread_mutex_unlock(&cost_lock);

	}
}

void* thread_3(){

	while (1){
	    dijkstra(cost, my_router_id); // calling the algo
	    print_lowestCost(); // printing all the lowest costs
	    sleep(10 + (rand() % 11)); // random 10 to 20 second sleep
    }
}

void print_lowestCost(){

    printf("\n\nLeast Cost Array: "); // title of values printing
    for(i = 0; i < nodes; ++i){
        printf("%d ", least_cost[i]); // print all the least costs
    }
    printf("\n");
}

void dijkstra(int cost_matrix[4][4], int source){

    // init variables
    int dist[4], visited[4] = {0}, m, min, start, d;

    for(i = 0; i < nodes; ++i)
        dist[i] = 10000; // init all best paths to some large number 
    
    dist[source] = 0; // init node of interest path value

    start = source; // init start to node of interest
    visited[source] = 1; // label node as visited
    dist[start] = 0; // init distance of node

    int count = nodes - 1; // init count for loop

    while(count > 0){
        min = 10000; // init min
        m = 0; // init m
        for(i = 0; i < nodes; ++i){    
            d = dist[start] + cost_matrix[start][i]; // as long as nodes calculate cost and dist
            if(d < dist[i] && visited[i] == 0){
                dist[i] = d; // if not visited and d is less that current distance, assign
            }
            if(min > dist[i] && visited[i] == 0){
                min = dist[i]; // if this new value is less than min, assign to min
                m = i; // assign index of node to m
            }
        }
        start = m; // init start to m
        visited[start] = 1; // label this node as visited
        --count; // decrement counter
    }

    for( i = 0; i < nodes; ++i){
        least_cost[i] = dist[i]; // store all distances in global least cost variable
    }
}

void print_cost_table(){

    printf("Cost Table Updated:\n"); // title of values printing
    for (i = 0; i < nodes; ++i){
        for ( j = 0; j < nodes; ++j){
            printf("%d\t",cost[i][j]); // print out 1 row of the cost matrix
        }
    printf("\n"); // print out a new line and print the next row
    }
}
