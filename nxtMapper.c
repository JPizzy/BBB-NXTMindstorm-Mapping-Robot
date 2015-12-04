#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include "udpListener.h"
#include "zencape.h"
#include <stdbool.h>

#define MAX_READS 5

#define M_PI 3.14159265359
#define JOYSTICK_PRESSED_PIN 27
#define JOYSTICK_UP_PIN 26
#define JOYSTICK_DOWN_PIN 46
#define JOYSTICK_LEFT_PIN 65
#define JOYSTICK_RIGHT_PIN 47
#define DISPLAY_LEFT_PIN 61
#define DISPLAY_RIGHT_PIN 44
#define FILEPATH_PRESSED_VALUE "/sys/class/gpio/gpio27/value" 
#define FILEPATH_UP_VALUE "/sys/class/gpio/gpio26/value" 
#define FILEPATH_DOWN_VALUE "/sys/class/gpio/gpio46/value" 
#define FILEPATH_LEFT_VALUE "/sys/class/gpio/gpio65/value" 
#define FILEPATH_RIGHT_VALUE "/sys/class/gpio/gpio47/value" 
#define FILEPATH_LEFTDISPLAY_VALUE "/sys/class/gpio61/value"
#define FILEPATH_RIGHTDISPLAY_VALUE "/sys/class/gpio44/value"

void calculateCoordinates(int cm, int angleIndex);

void exportPin(int pin)
{
	FILE *pfile = fopen("/sys/class/gpio/export", "w");
	if (pfile == NULL) {
		printf("ERROR: Unable to open export file.\n");
		exit(1);
	}
	
	fprintf(pfile, "%d", pin);
	fclose(pfile);
}

void initializePins()
{
	exportPin(JOYSTICK_PRESSED_PIN);
	exportPin(JOYSTICK_UP_PIN);
	exportPin(JOYSTICK_DOWN_PIN);
	exportPin(JOYSTICK_LEFT_PIN);
	exportPin(JOYSTICK_RIGHT_PIN);
    exportPin(DISPLAY_LEFT_PIN);
    exportPin(DISPLAY_RIGHT_PIN);
}

//Socket for bluetooth
static int s;

/******************************************************
 * Structs
 ******************************************************/

//Sends command to motor
typedef struct {
	uint8_t lsb;               //0x0C
	uint8_t msb;               //0x00
	uint8_t response;          //Response = 0x00, no Response = 0x80
	uint8_t command;           //Write = 0x04
	uint8_t port;              //0-2, 0xFF for ALL 
	uint8_t power;             //Range: -100 - 100 == 0x9C - 0x64
	uint8_t mode;              //Mode byte: motor on = 0x01, brake = 0x02, regulated = 0x04
	uint8_t reg;               //Regulation mode: idle = 0x00, motor speed = 0x01, motor sync 0x02
	uint8_t ratio;             //Turn Ratio: -100 - 100 == 0x9C - 0x64
	uint8_t state;             //Run state: idle = 0x00, ramp UP = 0x10, running = 0x20, ramp DOWN = 0x40
	uint8_t tach1;
	uint8_t tach2;
	uint8_t tach3;
	uint8_t tach4;
}__attribute__((packed)) setMotor;

//Struct for motor to return the current state of the motor specified
typedef struct {
	uint8_t lsb;               //0x03
	uint8_t msb;               //0x00
	uint8_t response;          //Response = 0x00, no Response = 0x80
	uint8_t command;           //Read = 0x06
	uint8_t port;              //Port range 0-2
}__attribute__((packed)) getMotor;

//Struct for the response package of the motor
typedef struct {
	uint8_t lsb;
	uint8_t msb;
	uint8_t response1;         //0x02
	uint8_t response2;         //0x06
	uint8_t status;            //Status Byte
	uint8_t port;              //Output Port
	uint8_t power;
	uint8_t mode;
	uint8_t reg;
	uint8_t ratio;
	uint8_t state;
	uint8_t tachLimit1;        //Current Limit on movement in progress if any
	uint8_t tachLimit2;
	uint8_t tachLimit3;
	uint8_t tachLimit4;
	uint8_t tachCount1;        //Number of coutns since last reset of motor counter
	uint8_t tachCount2;
	uint8_t tachCount3;
	uint8_t tachCount4;
	uint8_t tachBlock1;        //Current position relative to last programmed movement
	uint8_t tachBlock2;
    uint8_t tachBlock3;
    uint8_t tachBlock4;
	uint8_t rotationCount1;    //Current position relative to last reset
	uint8_t rotationCount2;
	uint8_t rotationCount3;
	uint8_t rotationCount4;
}__attribute__((packed)) motorResponse;

//Generic response struct for activating a motor or sensor
typedef struct {
	uint8_t lsb;
	uint8_t msb;
	uint8_t response;
	uint8_t command;
	uint8_t status;
}__attribute__((packed)) commandResponse;

//sensor activation struct
typedef struct {
	uint8_t lsb;               //0x05
	uint8_t msb;               //0x00
	uint8_t response;          //Response = 0x00, no Response = 0x80
	uint8_t command;           //Write = 0x05
	uint8_t port;              //Input port range 0-3
	uint8_t sensorType;        //Lowspeed_9v = 0x0B
	uint8_t sensorMode;        //Raw = 0x00
}__attribute__((packed)) setSensor;

//Struct for sensor to return the current state of the sensor specified
typedef struct {
	uint8_t lsb;               //0x03
	uint8_t msb;               //0x00
	uint8_t response;          //Response = 0x00, no Response = 0x80
	uint8_t read;              //Read = 0x07
	uint8_t port;              //Port = 0-3
}__attribute__((packed)) getSensor;

//Struct for the response package of the specified sensor
typedef struct {
	uint8_t lsb;
	uint8_t msb;
	uint8_t response1;         //0x02
	uint8_t response2;         //0x07
	uint8_t status;            //Status byte
	uint8_t port;              //Output port
	uint8_t isValid;           //Boolean: True if new data value
	uint8_t isCalibrated;      //Boolean: True if calibration file found for calibrated value
	uint8_t sensorType;
	uint8_t sensorMode;
	uint8_t rawValue1;         //Raw A/D value, device dependant
	uint8_t rawValue2;
	uint8_t normValue1;        //Normalized A/D value, Range 0 - 1023
	uint8_t normValue2;
	uint8_t scaledValue1;      //SWORD, mode dependant
	uint8_t scaledValue2;
	uint8_t calibratedValue1;  //Currently Unused
	uint8_t calibratedValue2;
}__attribute__((packed)) sensorResponse;

//Struct to get status for low speed
typedef struct {
	uint8_t lsb;               //0x03
	uint8_t msb;               //0x00
	uint8_t response;          //Response = 0x00, no Response = 0x80
	uint8_t command;           //Get status = 0x0E
	uint8_t port;              //Port range 0-3
}__attribute__((packed)) lsGetStatus;

//Struct for return package of status
typedef struct {
	uint8_t lsb;               //0x04
	uint8_t msb;               //0x00
	uint8_t response1;         //0x02
	uint8_t response2;         //0x0E
	uint8_t status;            //Status byte
	uint8_t bytesReady;        //Bytes ready (count of avaible bytes to read)
}__attribute__((packed)) lsGetStatusResponse;

//Struct for write command for low speed
typedef struct {
	uint8_t lsb;               //0x07
	uint8_t msb;               //0x00
	uint8_t response;          //Response = 0x00, no Response = 0x80
	uint8_t command;           //Write = 0x0F
	uint8_t port;              //Port range 0-3
	uint8_t txDataLength;      //Tx Data length (bytes)
	uint8_t rxDataLength;      //Rx Data length (bytes)
	uint8_t dataSent1;         //Tx Data
	uint8_t dataSent2;
}__attribute__((packed)) lsWrite;

//Struct for read command for low speed
typedef struct {
	uint8_t lsb;               //0x03
	uint8_t msb;               //0x00
	uint8_t response;          //Response = 0x00, no Response = 0x80
	uint8_t command;           //Read = 0x10
	uint8_t port;              //Port range 0-3
}__attribute__((packed)) lsRead;

//Struct for the response package of the low speed read.
typedef struct {
	uint8_t lsb;               //0x14
	uint8_t msb;               //0x00
	uint8_t response;          //0x02
	uint8_t command;           //0x10
	uint8_t status;            //Status byte
	uint8_t bytesRead;         //Bytes read
	uint8_t data1;             //Data padded
    uint8_t data2;
    uint8_t data3;
    uint8_t data4;
    uint8_t data5;
    uint8_t data6;
    uint8_t data7;
    uint8_t data8;
    uint8_t data9;
    uint8_t data10;
    uint8_t data11;
    uint8_t data12;
    uint8_t data13;
    uint8_t data14;
    uint8_t data15;
    uint8_t data16;
}__attribute__((packed)) lsReadReturn;

/******************************************************
 * Motor constants
 ******************************************************/
static uint8_t speedForward = 0x64;
static uint8_t speedReverse = 0x9C;

#define MOTOR_LEFT  0x00
#define MOTOR_RIGHT 0x02
#define TACH1 0x0F
#define TACH2 0x00
#define MOTOR_MODE 0x01
#define MOTOR_REG 0x00
#define SENSOR_PORT 0x00

static int distance = 0;
static bool mapDataReady = false;
static double coordinateX;
static double coordinateY;

bool isMapDataReady() {
	return mapDataReady;
}

void setMapDataRecieved() {
	mapDataReady = false;
}

int getDistanceValue() {
	mapDataReady = false;
	return distance;
}

double getXCoordinate() {
	return coordinateX;
}

double getYCoordinate() {
	return coordinateY;
}



void scanArea() {
	printf("Starting scan");
    setMotor rotateSensor = {
        0x0C,   //LSB
        0x00,   //MSB
        0x80,   //Response
        0x04,   //Write
        0x01,   //Port
        0x02,   //Power
        0x07,   //mode
        0x01,   //regulation
        0x00,   //ratio
        0x20,   //State
        0x0A,   //tach
        0x00,
        0x00,
        0x00
    };
    setMotor resetSensor = {
        0x0C,   //LSB
        0x00,   //MSB
        0x80,   //Response
        0x04,   //Write
        0x01,   //Port
        0xFE,   //Power
        0x07,   //mode
        0x01,   //regulation
        0x00,   //ratio
        0x20,   //State
        0x68,
        0x01,
        0x00,
        0x00
    };

    setSensor eyeWrite = {
        0x05, // LSB
        0x00, // MSB
        0x80, // Command: Direct command requires no response
        0x05, // Command Type: Set Input Mode
        SENSOR_PORT, // Sensor Port: Port 1
        0x0B, // Sensor Type: Lowspeed 9V
        0x00  // Sensor Mode: Raw
    };  

    lsRead ls_read = {
        0x03,
        0x00,
        0x00,
        0x10,
        SENSOR_PORT
    };

    lsReadReturn lsReadResponse;
    
    lsWrite lsWriteCmd = {
        0x07,
        0x00,
        0x80, // Command Type
        0x0F, // Command
        SENSOR_PORT, // Port
        0x02, // Bytes to write
        0x01, // Bytes to read
        0x02, // I2C address of device
        0x42  // Address of register
    };
    //Turn on Sensor
    if(!write(s, (const void *)&eyeWrite, (int)sizeof(eyeWrite))) {
        printf("Sensor not turned on! Requires more foreplay!\n\n");
    }
    
    long delayInSeconds = 0;
	long delayInNanoseconds = 500000000;
	bool validDistance;
	int readingCounter;

    for(int i = 0; i < 36; i ++) {
    	readingCounter = 0;
    	validDistance = false;
        if(!write(s, (const void *)&rotateSensor, (int)sizeof(rotateSensor))) {
            printf("Motor did not rotate\n\n");
        }
        
        nanosleep((const struct timespec[]){{delayInSeconds, delayInNanoseconds}}, NULL);
        //sleep(1);
    	
    	while(!validDistance && readingCounter < MAX_READS) {
        	if(!write(s, (const void *)&lsWriteCmd, (int)sizeof(lsWriteCmd))) {
            	printf("Sensor write did not work!\n\n");
        	}
    
    		nanosleep((const struct timespec[]){{delayInSeconds, delayInNanoseconds}}, NULL);
        	//sleep(1); // wait for sensor
        
        
        	// Actually read the sensor
        	if(!write(s, (const void *)&ls_read, (int)sizeof(ls_read))) {
            	printf("Sensor read did not work!\n\n");
        	}
        
        	// Store the reading into the read struct
        	if(!read(s, (void*)&lsReadResponse, (int)sizeof(lsReadResponse))) {
           	 printf("Read into struct did not work!\n\n");
       		}
       		if(!validDistance) {
        		distance = lsReadResponse.data1;
        		if(distance < 3 || distance > 175) {
        			readingCounter++;
        		}
        		else {
        			validDistance = true;
        		}
        	}
        }
        if(!validDistance) {
			distance = 255;
		}
		//Send to mapper
		printf("%d\n", distance);
		while(mapDataReady) {printf("Waiting for data"); sleep(1);}
		calculateCoordinates(distance, i);
		mapDataReady = true;
		
		setDisplay(distance);
    }

    if(write(s, (const void *)&resetSensor, (int)sizeof(resetSensor))) {
            printf("Motor rotating success\n\n");
        } else {
            printf("Motor did not rotate back to start\n\n");
        }
    sleep(10);
}

void nxtMove(int value) {
int speed = getSpeed();
speedForward = speed;
speedReverse = 256 - speed;
setMotor forwardA = {
            0x0C,
            0x00,
            0x80,
            0x04,
            MOTOR_LEFT,
            speedForward,
            MOTOR_MODE,
            MOTOR_REG,
            0x00,
            0x20,
            TACH1,
            TACH2,
            0x00,
            0x00
        };

setMotor reverseA = {
            0x0C,
            0x00,
            0x80,
            0x04,
            MOTOR_LEFT,
            speedReverse,
            MOTOR_MODE,
            MOTOR_REG,
            0x00,
            0x20,
            TACH1,
            TACH2,
            0x00,
            0x00
        };

setMotor forwardC = {
            0x0C,
            0x00,
            0x80,
            0x04,
            MOTOR_RIGHT,
            speedForward,
            MOTOR_MODE,
            MOTOR_REG,
            0x00,
            0x20,
            TACH1,
            TACH2,
            0x00,
            0x00
        };

setMotor reverseC = {
            0x0C,
            0x00,
            0x80,
            0x04,
            MOTOR_RIGHT,
            speedReverse,
            MOTOR_MODE,
            MOTOR_REG,
            0x00,
            0x20,
            TACH1,
            TACH2,
            0x00,
            0x00
    };
    if (value == 1) {
        //Forward
        write(s, (const void *)&forwardA, (int)sizeof(forwardA));
        write(s, (const void *)&forwardC, (int)sizeof(forwardC));
    } else if (value == 2) {
        //right
        write(s, (const void *)&forwardA, (int)sizeof(forwardA));
        write(s, (const void *)&reverseC, (int)sizeof(reverseC));

    } else if (value == 3) {
        //reverse
        write(s, (const void *)&reverseA, (int)sizeof(reverseA));
        write(s, (const void *)&reverseC, (int)sizeof(reverseC));
    } else if (value == 4) {
        //left
        write(s, (const void *)&forwardC, (int)sizeof(forwardC));
        write(s, (const void *)&reverseA, (int)sizeof(reverseA));

    } else if (value == 5) {
        scanArea();
    }
}


double degreesToRadians(int degrees) {
	return degrees * (M_PI/180.0);
}

void calculateCoordinates(int cm, int angleIndex) {
	if(angleIndex == 0) {
		coordinateX = 0;
		coordinateY = cm;
	}
	else if(angleIndex > 0 && angleIndex < 9) {
		coordinateX = (-1) * cm * sin(degreesToRadians((angleIndex * 10)));
		coordinateY = cm * cos(degreesToRadians((angleIndex * 10)));
	}
	else if(angleIndex == 9) {
		coordinateX = -cm;
		coordinateY = 0;
	}
	else if(angleIndex > 9 && angleIndex < 18) {
		coordinateX = (-1) * cm * cos(degreesToRadians(((angleIndex - 9) * 10)));
		coordinateY = (-1) * cm * sin(degreesToRadians(((angleIndex - 9) * 10)));
	}
	else if(angleIndex == 18) {
		coordinateX = 0;
		coordinateY = -cm;
	}
	else if(angleIndex > 18 && angleIndex < 27) {
		coordinateX = cm * sin(degreesToRadians(((angleIndex - 18) * 10)));
		coordinateY = (-1) * cm * cos(degreesToRadians(((angleIndex - 18) * 10)));
	}
	else if(angleIndex == 27) {
		coordinateX = cm;
		coordinateY = 0;
	} else if (angleIndex > 27) {
		coordinateX = cm * cos(degreesToRadians(((angleIndex - 27) * 10)));
		coordinateY = cm * sin(degreesToRadians(((angleIndex - 27) * 10)));
	}
}

int main(int argc, char **argv)
{

	initializePins();

    struct sockaddr_rc addr = { 0 };
    int status;
    const char dest[18] = "00:16:53:09:D3:3C";

    // allocate a socket
    s = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);

    // set the connection parameters (who to connect to)
    addr.rc_family = AF_BLUETOOTH;
    addr.rc_channel = (uint8_t) 1;
    str2ba( dest, &addr.rc_bdaddr );

    // connect to server
    status = connect(s, (struct sockaddr *)&addr, sizeof(addr));
       
    pthread_t joystickThread;
    pthread_t displayThread;

    // send a message
    printf("status: %d\n", status);

    // send a message
    if( status == 0 ) {
        // start udp listener
        UDPListener_launchThread();
        pthread_create(&joystickThread, NULL, joystickStart, NULL);
        pthread_create(&displayThread, NULL, displayStart, NULL);
        while(1) {

        }    
    }
    
    if( status < 0 ) perror("Error");

    close(s);
    return 0;
}
