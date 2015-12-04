#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <math.h>
#include <pthread.h>
#include <stdbool.h>

#include "zencape.h"
#include "nxtMapper.h"

/******************************************************
 * Potentiometer
 ******************************************************/
#define A2D_FILE_VOLTAGE0  "/sys/bus/iio/devices/iio:device0/in_voltage0_raw"
#define FILEPATH_DEVICE_SLOTS "/sys/devices/bone_capemgr.9/slots"

void slotA2D() {
	FILE *slotsFile = fopen(FILEPATH_DEVICE_SLOTS, "w");
	if (slotsFile == NULL) {
		printf("ERROR: Unable to open file: %s\n", FILEPATH_DEVICE_SLOTS);
		exit(-1);
	}
	
	fprintf(slotsFile, "%s", "BB-ADC");
	fclose(slotsFile);
}

float getVoltageReading()
{
	// Open file
	FILE *f = fopen(A2D_FILE_VOLTAGE0, "r");
	if (!f) {
		printf("ERROR: Unable to open voltage input file. Cape loaded?\n");
		printf("try:	echo BB-ADC > /sys/devices/bone_capemgr.9/slots\n");
		exit(-1);
	}

	// Get reading
	float a2dReading = 0;
	float itemsRead = fscanf(f, "%f", &a2dReading);
	if (itemsRead <= 0) {
		printf("ERROR: Unable to read values from voltage input file.\n");
		exit(-1);
	}

	// Close file
	fclose(f);

	return a2dReading;
}

int getSpeed() {
	float measuredValue = getVoltageReading();
	int value;
	value = ((measuredValue - 0)/(4096))*99 + 1;
	return value;
}

/******************************************************
 * 14 Seg Display
 ******************************************************/
#define I2CDRV_LINUX_BUS0 "/dev/i2c-0"
#define I2CDRV_LINUX_BUS1 "/dev/i2c-1"
#define I2CDRV_LINUX_BUS2 "/dev/i2c-2"

#define I2C_DEVICE_ADDRESS 0x20

#define REG_DIRA 0x00
#define REG_DIRB 0x01
#define REG_BOT 0x14 //lower
#define REG_TOP 0x15 //upper

//The Hex codes to make numbers 1-0
#define BOT_ONE 0x80
#define TOP_ONE 0x12
#define BOT_TWO 0x31
#define TOP_TWO 0x0E
#define BOT_THREE 0xB0
#define TOP_THREE 0x0E
#define BOT_FOUR 0x90
#define TOP_FOUR 0x8A
#define BOT_FIVE 0xB0
#define TOP_FIVE 0x8C
#define BOT_SIX 0xB1
#define TOP_SIX 0x8C
#define BOT_SEV 0x80
#define TOP_SEV 0x06
#define BOT_EIG 0xB1
#define TOP_EIG 0x8E
#define BOT_NINE 0xB0
#define TOP_NINE 0x8E
#define BOT_ZERO 0xA1
#define TOP_ZERO 0x86

static int initI2cBus(char* bus, int address);
static void writeI2cReg(int i2cFileDesc, unsigned char regAddr, unsigned char value);
void slotI2C();

static int i2cFileDesc;
static int leftDigit = 0;
static int rightDigit = 0;

void setDisplayToOutput() {
	FILE *leftDigitFile = fopen("/sys/class/gpio/gpio44/direction", "w");
	if (leftDigitFile == NULL) {
		printf("ERROR: Unable to open export file.\n");
		exit(1);
	}
	FILE *rightDigitFile = fopen("/sys/class/gpio/gpio61/value", "w");
	if (rightDigitFile == NULL) {
		printf("ERROR: Unable to open export file.\n");
		exit(1);
	}
	fprintf (leftDigitFile, "out");
	fprintf (rightDigitFile, "out");
	fclose (leftDigitFile);
	fclose (rightDigitFile);
}


//Actiavtes the left display;
void activateLeft() {
	FILE *fileName = fopen("/sys/class/gpio/gpio61/value", "w");
	if (fileName == NULL) {
		printf("ERROR: Unable to open export file.\n");
		exit(1);
	}
	fprintf (fileName, "%d", 1);
	fclose (fileName);
}
//Actiavtes right display
void activateRight() {
	FILE *fileName = fopen("/sys/class/gpio/gpio44/value", "w");
	if (fileName == NULL) {
		printf("ERROR: Unable to open export file.\n");
		exit(1);
	}
	fprintf (fileName, "%d", 1);
	fclose (fileName);
}
//Turns off left
void deActivateLeft() {
	FILE *fileName = fopen("/sys/class/gpio/gpio61/value", "w");
	if (fileName == NULL) {
		printf("ERROR: Unable to open export file.\n");
		exit(1);
	}
	fprintf (fileName, "%d", 0);
	fclose (fileName);
}
//Turns off right
void deActivateRight() {
	FILE *fileName = fopen("/sys/class/gpio/gpio44/value", "w");
	if (fileName == NULL) {
		printf("ERROR: Unable to open export file.\n");
		exit(1);
	}
	fprintf (fileName, "%d", 0);
	fclose (fileName);
}

void setDisplay(int distance) {
	if (distance > 100) {
		leftDigit = 9;
		rightDigit = 9;

	} else {
		leftDigit = distance/10;
		rightDigit = distance%10;
	}
}

void driveRight() {

	if(rightDigit == 0) {
		writeI2cReg(i2cFileDesc, REG_BOT, BOT_ZERO);
		writeI2cReg(i2cFileDesc, REG_TOP, TOP_ZERO);
	}
	if(rightDigit == 1) {
		writeI2cReg(i2cFileDesc, REG_BOT, BOT_ONE);
		writeI2cReg(i2cFileDesc, REG_TOP, TOP_ONE);
	}
	if(rightDigit == 2) {
		writeI2cReg(i2cFileDesc, REG_BOT, BOT_TWO);
		writeI2cReg(i2cFileDesc, REG_TOP, TOP_TWO);	
	}
	if(rightDigit == 3) {
		writeI2cReg(i2cFileDesc, REG_BOT, BOT_THREE);
		writeI2cReg(i2cFileDesc, REG_TOP, TOP_THREE);	
	}
	if(rightDigit == 4) {
		writeI2cReg(i2cFileDesc, REG_BOT, BOT_FOUR);
		writeI2cReg(i2cFileDesc, REG_TOP, TOP_FOUR);	
	}
	if(rightDigit == 5) {
		writeI2cReg(i2cFileDesc, REG_BOT, BOT_FIVE);
		writeI2cReg(i2cFileDesc, REG_TOP, TOP_FIVE);	
	}
	if(rightDigit == 6) {
		writeI2cReg(i2cFileDesc, REG_BOT, BOT_SIX);
		writeI2cReg(i2cFileDesc, REG_TOP, TOP_SIX);	
	}
	if(rightDigit == 7) {
		writeI2cReg(i2cFileDesc, REG_BOT, BOT_SEV);
		writeI2cReg(i2cFileDesc, REG_TOP, TOP_SEV);	
	}
	if(rightDigit == 8) {
		writeI2cReg(i2cFileDesc, REG_BOT, BOT_EIG);
		writeI2cReg(i2cFileDesc, REG_TOP, TOP_EIG);	
	}
	if(rightDigit == 9) {
		writeI2cReg(i2cFileDesc, REG_BOT, BOT_NINE);
		writeI2cReg(i2cFileDesc, REG_TOP, TOP_NINE);	
	}
	long seconds = 0;
	long nanoseconds = 5000000;
	struct timespec reqDelay = {seconds, nanoseconds};
	activateRight();
	nanosleep(&reqDelay, (struct timespec *) NULL);
	deActivateRight();
}

void driveLeft() {
	if(leftDigit == 0) {
		writeI2cReg(i2cFileDesc, REG_BOT, BOT_ZERO);
		writeI2cReg(i2cFileDesc, REG_TOP, TOP_ZERO);
	}
	if(leftDigit == 1) {
		writeI2cReg(i2cFileDesc, REG_BOT, BOT_ONE);
		writeI2cReg(i2cFileDesc, REG_TOP, TOP_ONE);
	}
	if(leftDigit == 2) {
		writeI2cReg(i2cFileDesc, REG_BOT, BOT_TWO);
		writeI2cReg(i2cFileDesc, REG_TOP, TOP_TWO);	
	}
	if(leftDigit == 3) {
		writeI2cReg(i2cFileDesc, REG_BOT, BOT_THREE);
		writeI2cReg(i2cFileDesc, REG_TOP, TOP_THREE);	
	}
	if(leftDigit == 4) {
		writeI2cReg(i2cFileDesc, REG_BOT, BOT_FOUR);
		writeI2cReg(i2cFileDesc, REG_TOP, TOP_FOUR);	
	}
	if(leftDigit == 5) {
		writeI2cReg(i2cFileDesc, REG_BOT, BOT_FIVE);
		writeI2cReg(i2cFileDesc, REG_TOP, TOP_FIVE);	
	}
	if(leftDigit == 6) {
		writeI2cReg(i2cFileDesc, REG_BOT, BOT_SIX);
		writeI2cReg(i2cFileDesc, REG_TOP, TOP_SIX);	
	}
	if(leftDigit == 7) {
		writeI2cReg(i2cFileDesc, REG_BOT, BOT_SEV);
		writeI2cReg(i2cFileDesc, REG_TOP, TOP_SEV);	
	}
	if(leftDigit == 8) {
		writeI2cReg(i2cFileDesc, REG_BOT, BOT_EIG);
		writeI2cReg(i2cFileDesc, REG_TOP, TOP_EIG);	
	}
	if(leftDigit == 9) {
		writeI2cReg(i2cFileDesc, REG_BOT, BOT_NINE);
		writeI2cReg(i2cFileDesc, REG_TOP, TOP_NINE);	
	}
	long seconds = 0;
	long nanoseconds = 5000000;
	struct timespec reqDelay = {seconds, nanoseconds};
	activateLeft();
	nanosleep(&reqDelay, (struct timespec *) NULL);
	deActivateLeft();
}

void *displayStart()
{

	slotA2D();
	slotI2C();
	setDisplayToOutput();
	
	i2cFileDesc = initI2cBus(I2CDRV_LINUX_BUS2, I2C_DEVICE_ADDRESS);

	writeI2cReg(i2cFileDesc, REG_DIRA, 0x00);
	writeI2cReg(i2cFileDesc, REG_DIRB, 0x00);

	//Clear Display
	writeI2cReg(i2cFileDesc, REG_BOT, 0x00);
	writeI2cReg(i2cFileDesc, REG_TOP, 0x00);

	deActivateRight();
	deActivateLeft();
	while(1) {
	//Do stuff here
		driveLeft();
		driveRight();
	}
	deActivateRight();
	deActivateLeft();
	close(i2cFileDesc);
}

void slotI2C() {
	FILE *slotsFile = fopen(FILEPATH_DEVICE_SLOTS, "w");
	if (slotsFile == NULL) {
		printf("ERROR: Unable to open file: %s\n", FILEPATH_DEVICE_SLOTS);
		exit(-1);
	}
	
	fprintf(slotsFile, "%s", "BB-I2C1");
	fclose(slotsFile);
}

//Dr. Brian Frasers code to open bus for read/write
static int initI2cBus(char* bus, int address)
{
	int i2cFileDesc = open(bus, O_RDWR);
	if (i2cFileDesc < 0) {
		printf("I2C DRV: Unable to open bus for read/write (%s)\n", bus);
		perror("Error is:");
		exit(-1);
	}

	int result = ioctl(i2cFileDesc, I2C_SLAVE, address);
	if (result < 0) {
		perror("Unable to set I2C device to slave address.");
		exit(-1);
	}
	return i2cFileDesc;
}

//Dr.Brian Frasers code to write into register
static void writeI2cReg(int i2cFileDesc, unsigned char regAddr, unsigned char value)
{
	unsigned char buff[2];
	buff[0] = regAddr;
	buff[1] = value;
	int res = write(i2cFileDesc, buff, 2);
	if (res != 2) {
		perror("Unable to write i2c register");
		exit(-1);
	}
}

/******************************************************
 * JoyStick
 ******************************************************/


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

    FILE *joystickPressed = fopen("/sys/class/gpio/gpio27/value", "r");
    if (joystickPressed == NULL) {
        printf("ERROR: Unable to open file for pressed read\n");
        exit(1);
    }
    fgets(buff, BUFFER_MAX, joystickPressed);
    fclose(joystickPressed);
    if (buff[0] == '0') {
        return 5;
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
 void *joystickStart() {
 	int value;
 	while(1) {
 		pollDelay();
 		value = checkInput();
 		if(value) {
 			nxtMove(value);
 			actionDelay();
 		}
 	}
 }
