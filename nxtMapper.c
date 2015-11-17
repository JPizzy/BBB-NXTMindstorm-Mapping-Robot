#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include "udpListener.h"

#define BUFFER_MAX 1024
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
int main(int argc, char **argv)
{
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
