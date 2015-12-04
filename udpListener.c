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
#include "zencape.h"

#define PORT 12345
#define MSG_MAX_LEN 1024

#define FORWARD 1
#define RIGHT 2
#define BACKWARD 3
#define LEFT 4
#define MAPPING 5

void *UDPListen(void* arg) {
	
	struct sockaddr_in sin;
	char message[MSG_MAX_LEN];
	char upTime[MSG_MAX_LEN] = {"/proc/uptime"};
	char beginMapping[MSG_MAX_LEN];
	char moveForward[MSG_MAX_LEN];
	char moveBackward[MSG_MAX_LEN];
	char moveLeft[MSG_MAX_LEN];
	char moveRight[MSG_MAX_LEN];
	char mapData[MSG_MAX_LEN];
	char power[MSG_MAX_LEN];
	int xCoordinate;
	int yCoordinate;
	int distanceVal;
	int powerVal;

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
		else if(strcmp(message, "beginMapping") == 0) {
			initiateMapper();
			if(sendto(socketDescriptor, beginMapping, strlen(beginMapping), 0,
				(struct sockaddr*)&sin, sin_len) == -1) {
					perror("sendto()");
					exit(1);
				}
		}
		else if(strcmp(message, "moveForward") == 0) {
			nxtMove(FORWARD);
			if(sendto(socketDescriptor, moveForward, strlen(moveForward), 0,
				(struct sockaddr*)&sin, sin_len) == -1) {
					perror("sendto()");
					exit(1);
				}
		}
		else if(strcmp(message, "moveBackward") == 0) {
			nxtMove(BACKWARD);
			if(sendto(socketDescriptor, moveBackward, strlen(moveBackward), 0,
				(struct sockaddr*)&sin, sin_len) == -1) {
					perror("sendto()");
					exit(1);
				}
		}
		else if(strcmp(message, "moveLeft") == 0) {
			nxtMove(LEFT);
			if(sendto(socketDescriptor, moveLeft, strlen(moveLeft), 0,
				(struct sockaddr*)&sin, sin_len) == -1) {
					perror("sendto()");
					exit(1);
				}
		}
		else if(strcmp(message, "moveRight") == 0) {
			nxtMove(RIGHT);
			if(sendto(socketDescriptor, moveRight, strlen(moveRight), 0,
				(struct sockaddr*)&sin, sin_len) == -1) {
					perror("sendto()");
					exit(1);
				}
		}
		else if(strcmp(message, "getPower") == 0) {
			powerVal = getSpeed();
			sprintf(power, "%d", powerVal);
			
			if(sendto(socketDescriptor, power, strlen(power), 0,
				(struct sockaddr*)&sin, sin_len) == -1) {
					perror("sendto()");
					exit(1);
				}
		}
		else if(strcmp(message, "getMapData") == 0) {
			if(isMapDataReady()) {
				// Change to double if more percision is required
				xCoordinate = getXCoordinate();
				yCoordinate = getYCoordinate();
				distanceVal = getDistanceValue();
				sprintf(mapData, "%d %d %d", distanceVal, xCoordinate, yCoordinate);
				
				if(sendto(socketDescriptor, mapData, strlen(mapData), 0,
				(struct sockaddr*)&sin, sin_len) == -1) {
					perror("sendto()");
					exit(1);
				}
				setMapDataRecieved();
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
