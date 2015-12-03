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

float getVoltageReading()
{
	// Open file
	FILE *f = fopen(A2D_FILE_VOLTAGE0, "r");
	if (!f) {
		printf("ERROR: Unable to open voltage input file. Cape loaded?\n");
		printf("try:	echo BB_ADC > /sys/devices/bone_capemgr.9/slots\n");
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
	//float measuredValue = getVoltageReading();
	return 0;
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

static int i2cFileDesc;

void *displayStart()
{
	i2cFileDesc = initI2cBus(I2CDRV_LINUX_BUS2, I2C_DEVICE_ADDRESS);

	writeI2cReg(i2cFileDesc, REG_DIRA, 0x00);
	writeI2cReg(i2cFileDesc, REG_DIRB, 0x00);

	//Clear Display
	writeI2cReg(i2cFileDesc, REG_BOT, 0x00);
	writeI2cReg(i2cFileDesc, REG_TOP, 0x00);

	while(1) {
	//Do stuff here

	}
	close(i2cFileDesc);
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