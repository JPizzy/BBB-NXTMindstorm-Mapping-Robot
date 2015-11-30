#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include "udpListener.h"

#define BUFFER_MAX 1024
#define JOYSTICK_PRESSED_PIN 27
#define JOYSTICK_UP_PIN 26
#define JOYSTICK_DOWN_PIN 46
#define JOYSTICK_LEFT_PIN 65
#define JOYSTICK_RIGHT_PIN 47
#define FILEPATH_PRESSED_VALUE "/sys/class/gpio/gpio27/value" 
#define FILEPATH_UP_VALUE "/sys/class/gpio/gpio26/value" 
#define FILEPATH_DOWN_VALUE "/sys/class/gpio/gpio46/value" 
#define FILEPATH_LEFT_VALUE "/sys/class/gpio/gpio65/value" 
#define FILEPATH_RIGHT_VALUE "/sys/class/gpio/gpio47/value" 

void exportJoystickPin(int pin)
{
	FILE *pfile = fopen("/sys/class/gpio/export", "w");
	if (pfile == NULL) {
		printf("ERROR: Unable to open export file.\n");
		exit(1);
	}
	
	fprintf(pfile, "%d", pin);
	fclose(pfile);
}

void initializeJoysticks()
{
	exportJoystickPin(JOYSTICK_PRESSED_PIN);
	exportJoystickPin(JOYSTICK_UP_PIN);
	exportJoystickPin(JOYSTICK_DOWN_PIN);
	exportJoystickPin(JOYSTICK_LEFT_PIN);
	exportJoystickPin(JOYSTICK_RIGHT_PIN);
}


//Socket for bluetooth
static int s;

//motor command struct
typedef struct {
	uint8_t lsb;
	uint8_t msb;
	uint8_t cmdType;
	uint8_t cmdState;
	uint8_t motor;
	uint8_t speed;
	uint8_t mode;
	uint8_t reg;
	uint8_t ratio;
	uint8_t state;
	uint8_t tach1;
	uint8_t tach2;
	uint8_t tach3;
	uint8_t tach4;
}__attribute__((packed)) setMotor;

typedef struct {
	uint8_t lsb;
	uint8_t msb;
	uint8_t response;
	uint8_t read;
	uint8_t port;
}__attribute__((packed)) getMotor;

typedef struct {
	uint8_t lsb;
	uint8_t msb;
	uint8_t response1;
	uint8_t response2;
	uint8_t power;
	uint8_t mode;
	uint8_t regMode;
	uint8_t turnRatio;
	uint8_t runState;
	uint8_t tach1;
	uint8_t tach2;
	uint8_t tach3;
	uint8_t tach4;
	uint8_t tachCount1;
	uint8_t tachCount2;
	uint8_t tachCount3;
	uint8_t tachCount4;
	uint8_t tachBlock1;
	uint8_t tachBlock2;
	uint8_t tachBlock3;
	uint8_t tachBlock4;
	uint8_t rotationCount1;
	uint8_t rotationCount2;
	uint8_t rotationCount3;
	uint8_t rotationCount4;
}__attribute__((packed)) motorResponse;

typedef struct {
	uint8_t lsb;
	uint8_t msb;
	uint8_t response;
	uint8_t command;
	uint8_t status;
}__attribute__((packed)) commandResponse;

//sensor command struct
typedef struct {
	uint8_t lsb;
	uint8_t msb;
	uint8_t cmdType;
	uint8_t cmdState;
	uint8_t port;
	uint8_t sensorType;
	uint8_t sensorMode;
}__attribute__((packed)) setSensor;

typedef struct {
	uint8_t lsb;
	uint8_t msb;
	uint8_t response;
	uint8_t read;
	uint8_t port;
}__attribute__((packed)) getSensor;

//sensor response command struct
typedef struct {
	uint8_t lsb;
	uint8_t msb;
	uint8_t response1;
	uint8_t response2;
	uint8_t sensorStatus;
	uint8_t port;
	uint8_t isValid;
	uint8_t isCalibrated;
	uint8_t sensorType;
	uint8_t sensorMode;
	uint8_t rawValue1;
	uint8_t rawValue2;
	uint8_t normValue1;
	uint8_t normValue2;
	uint8_t scaledValue1;
	uint8_t scaledValue2;
	uint8_t calibratedValue1;
	uint8_t calibratedValue2;
}__attribute__((packed)) sensorResponse;

void activatePin() {
    FILE *joystickFile = fopen("/sys/class/gpio/export", "w");
    if (joystickFile == NULL) {
        printf("ERROR: Unable to open export file.\n");
        exit(1);
    }
    fprintf(joystickFile, "%d", 26);
    fprintf(joystickFile, "%d", 47);
    fprintf(joystickFile, "%d", 46);
    fprintf(joystickFile, "%d", 65);
    fprintf(joystickFile, "%d", 27);
    fclose(joystickFile);
}

int checkInput() {
    char buff[BUFFER_MAX];

    //Up
    FILE *joystickUp = fopen("/sys/class/gpio/gpio26/value", "r");
    if (joystickUp == NULL) {
        printf("ERROR: Unable to open file for up read\n");
        exit(1);
    }
    fgets(buff, BUFFER_MAX, joystickUp);
    fclose(joystickUp);
    if (buff[0] == '0') {
        return 1;
    }

    //Right
    FILE *joystickRight = fopen("/sys/class/gpio/gpio47/value", "r");
    if (joystickRight == NULL) {
        printf("ERROR: Unable to open file for right read\n");
        exit(1);
    }
    fgets(buff, BUFFER_MAX, joystickRight);
    fclose(joystickRight);
    if (buff[0] == '0') {
        return 2;
    }

    //Down
    FILE *joystickDown = fopen("/sys/class/gpio/gpio46/value", "r");
    if (joystickDown == NULL) {
        printf("ERROR: Unable to open file for down read\n");
        exit(1);
    }
    fgets(buff, BUFFER_MAX, joystickDown);
    fclose(joystickDown);
    if (buff[0] == '0') {
        return 3;
    }

    //Left
    FILE *joystickLeft = fopen("/sys/class/gpio/gpio65/value", "r");
    if (joystickLeft == NULL) {
        printf("ERROR: Unable to open file for left read\n");
        exit(1);
    }
    fgets(buff, BUFFER_MAX, joystickLeft);
    fclose(joystickLeft);
    if (buff[0] == '0') {
        return 4;
    }

    return 0;
}

//Wait 10ms
void pollDelay() {
    long seconds = 0;
    long nanoseconds = 10000000;
    struct timespec reqDelay = {seconds, nanoseconds};
    nanosleep(&reqDelay, (struct timespec *) NULL);
}

//Wait 100ms
void actionDelay() {
    long seconds = 0;
    long nanoseconds = 100000000;
    struct timespec reqDelay = {seconds, nanoseconds};
    nanosleep(&reqDelay, (struct timespec *) NULL);
}

void nxtMove(int value) {
    if (value == 1) {
        //Forward
        setMotor startA = {
            0x0C,
            0x00,
            0x80,
            0x04,
            0x00,
            0x64,
            0x01,
            0x00,
            0x00,
            0x20,
            0x0F,
            0x00,
            0x00,
            0x00
        };

        setMotor startC = {
            0x0C,
            0x00,
            0x80,
            0x04,
            0x02,
            0x64,
            0x01,
            0x00,
            0x00,
            0x20,
            0x0F,
            0x00,
            0x00,
            0x00
        };
        write(s, (const void *)&startA, (int)sizeof(startA));
        write(s, (const void *)&startC, (int)sizeof(startC));
    } else if (value == 2) {
        //right
        setMotor startA = {
            0x0C,
            0x00,
            0x80,
            0x04,
            0x00,
            0x64,
            0x01,
            0x00,
            0x00,
            0x20,
            0x0F,
            0x00,
            0x00,
            0x00
        };
        setMotor startC = {
            0x0C,
            0x00,
            0x80,
            0x04,
            0x02,
            0x9C,
            0x01,
            0x00,
            0x00,
            0x20,
            0x0F,
            0x00,
            0x00,
            0x00  
        };
        write(s, (const void *)&startA, (int)sizeof(startA));
        write(s, (const void *)&startC, (int)sizeof(startC));

    } else if (value == 3) {
        //reverse
        setMotor startA = {
            0x0C,
            0x00,
            0x80,
            0x04,
            0x00,
            0x9C,
            0x01,
            0x00,
            0x00,
            0x20,
            0x0F,
            0x00,
            0x00,
            0x00
        };

        setMotor startC = {
            0x0C,
            0x00,
            0x80,
            0x04,
            0x02,
            0x9C,
            0x01,
            0x00,
            0x00,
            0x20,
            0x0F,
            0x00,
            0x00,
            0x00  
        };
        write(s, (const void *)&startA, (int)sizeof(startA));
        write(s, (const void *)&startC, (int)sizeof(startC));
    } else if (value == 4) {
        //left
        setMotor startA = {
            0x0C,
            0x00,
            0x80,
            0x04,
            0x00,
            0x9C,
            0x01,
            0x00,
            0x00,
            0x20,
            0x0F,
            0x00,
            0x00,
            0x00
        };
        setMotor startC = {
            0x0C,
            0x00,
            0x80,
            0x04,
            0x02,
            0x64,
            0x01,
            0x00,
            0x00,
            0x20,
            0x0F,
            0x00,
            0x00,
            0x00
        };
        write(s, (const void *)&startA, (int)sizeof(startA));
        write(s, (const void *)&startC, (int)sizeof(startC));
    }

}

void *EyeSensor(void* arg) {
	printf("Started eye sensor thread!\n");
	sensorResponse eyeRead = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	setSensor eyeWrite = {
		0x05, // LSB
        0x00, // MSB
        
        0x80, // Command: Direct command requires no response
        0x05, // Command Type: Set Input Mode
        0x01, // Sensor Port: Port 1
        0x0B, // Sensor Type: Lowspeed 9V
        0x00  // Sensor Mode: Raw
	};
	//commandResponse commandRes;
	getSensor getResponse = {
		0x03, // LSB
		0x00, // MSB
		
		0x00, // Command: Direct command require response
		0x07, // Command Type: Get Input Values
		0x01  // Sensor Port: Port 1
	};
	
	// Turn on sensor
	if(write(s, (const void *)&eyeWrite, (int)sizeof(eyeWrite))) {
		printf("Sensor turned on sucessfully!\n\n");
	}
	else {
		printf("Sensor not turned on! Requires foreplay!\n\n");
	}
	
	while(1){
	
		sleep(1);
		
		// Poll sensor
		if(write(s, (const void *)&getResponse, (int)sizeof(getResponse))) {
			printf("Sensor write sucessful!\n\n");
		}
		else {
			printf("Sensor write did not work!\n\n");
		}
		
		// Read sensor
		printf("Attempting read\n");
		if(read(s, (void*)&eyeRead, (int)sizeof(eyeRead))) {
			printf("Sensor read sucessful!\n\n");
		}
		else {
			printf("Sensor read did not work!\n\n");
		}
		
		printf("\
LSB:         %x\n \
MSB:         %x\n \
0x02:        %x\n \
0x07:        %x\n \
Status:      %x\n \
Input Port:  %x\n \
Valid:       %x\n \
Calibrated:  %x\n \
Sensor Type: %x\n \
Sensor Mode: %x\n \
RAW:         %x\n \
RAW:         %x\n \
Normalized:  %x\n \
Normalized:  %x\n \
Scaled:      %x\n \
Scaled:      %x\n \
Calibrated:  %x\n \
Calibrated:  %x\n",
		eyeRead.lsb,
		eyeRead.msb,
		eyeRead.response1,
		eyeRead.response2,
		eyeRead.sensorStatus,
		eyeRead.port,
		eyeRead.isValid,
		eyeRead.isCalibrated,
		eyeRead.sensorType,
		eyeRead.sensorMode,
		eyeRead.rawValue1,
		eyeRead.rawValue2,
		eyeRead.normValue1,
		eyeRead.normValue2,
		eyeRead.scaledValue1,
		eyeRead.scaledValue2,
		eyeRead.calibratedValue1,
		eyeRead.calibratedValue2);
		//printf("Raw: (%x, %x)", eyeRead.rawValue1, eyeRead.rawValue2);
	/*printf("%x\n %x\n %x\n %x\n %x\n", commandRes.lsb, commandRes.msb, commandRes.response, commandRes.command, commandRes.status);*/
	}
}

int main(int argc, char **argv)
{

	initializeJoysticks();

    struct sockaddr_rc addr = { 0 };
    int status;
    int value = 0;
    activatePin();
    const char dest[18] = "00:16:53:09:D3:3C";

    // allocate a socket
    s = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);

    // set the connection parameters (who to connect to)
    addr.rc_family = AF_BLUETOOTH;
    addr.rc_channel = (uint8_t) 1;
    str2ba( dest, &addr.rc_bdaddr );

    // connect to server
    status = connect(s, (struct sockaddr *)&addr, sizeof(addr));
    
    // start udp listener
    UDPListener_launchThread();
    
    pthread_t EyeSensorThread;
	pthread_create(&EyeSensorThread, NULL, EyeSensor, NULL);

    // send a message
    printf("status: %d\n", status);

    // send a message
    if( status == 0 ) {
        while(1) {
            pollDelay();
            value = checkInput();
            if(value) {
                nxtMove(value);
                actionDelay();
            }
        }
    }
    
    

    if( status < 0 ) perror("Error");

    close(s);
    return 0;
}
