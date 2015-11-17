#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include "nxtMapper.h"

#define PORT 12345
#define MSG_MAX_LEN 1024

void *UDPListen(void* arg) {
	
	struct sockaddr_in sin;
	char message[MSG_MAX_LEN];
	char upTime[MSG_MAX_LEN] = {"/proc/uptime"};

	socklen_t sin_len = sizeof(sin);
	int bytesRx;

	// Creates the socket here
	int socketDescriptor = socket(PF_INET, SOCK_DGRAM, 0);

	// Adds the port number 12345
	sin.sin_port = htons(PORT);

	// Binds socket here
	bind(socketDescriptor, (struct sockaddr*)&sin, sizeof(sin));

	// Waits for commands and loops until quit
	while(1) {
		
		fflush(stdout);

		// Receive data here
		if((bytesRx = recvfrom(socketDescriptor, message, MSG_MAX_LEN, 0,
			(struct sockaddr*)&sin, &sin_len)) == -1) {
		
			perror("recvfrom()");
			exit(1);
		}

		if(strcmp(message, "uptime") == 0) {
			if(sendto(socketDescriptor, upTime, strlen(upTime), 0,
				(struct sockaddr*)&sin, sin_len) == -1) {
					perror("sendto()");
					exit(1);
				}
		}
		else if(strcmp(message, "moveForward") == 0) {
			// TODO: Insert movement function here
			// NxtMapper_moveForward();
			nxtMove(1);
			if(sendto(socketDescriptor, volume, strlen(volume), 0,
				(struct sockaddr*)&sin, sin_len) == -1) {
					perror("sendto()");
					exit(1);
				}
		}
		else if(strcmp(message, "moveBackward") == 0) {
			// TODO: Insert movement function here
			// NxtMapper_moveBackward();
			nxtMove(3);
			if(sendto(socketDescriptor, volume, strlen(volume), 0,
				(struct sockaddr*)&sin, sin_len) == -1) {
					perror("sendto()");
					exit(1);
				}
		}
		else if(strcmp(message, "moveLeft") == 0) {
			// TODO: Insert movement function here
			// NxtMapper_turnLeft();
			nxtMove(4);
			if(sendto(socketDescriptor, volume, strlen(volume), 0,
				(struct sockaddr*)&sin, sin_len) == -1) {
					perror("sendto()");
					exit(1);
				}
		}
		else if(strcmp(message, "moveRight") == 0) {
			// TODO: Insert movement function here
			// NxtMapper_turnRight();
			nxtMove(3);
			if(sendto(socketDescriptor, volume, strlen(volume), 0,
				(struct sockaddr*)&sin, sin_len) == -1) {
					perror("sendto()");
					exit(1);
				}
		}

		// Clears the char array
		memset(message, '\0', MSG_MAX_LEN);
	}

	close(socketDescriptor);
	
	return NULL;
}

void UDPListener_launchThread() {
	pthread_t UDPListenerThread;
	pthread_create(&UDPListenerThread, NULL, UDPListen, NULL);
}
