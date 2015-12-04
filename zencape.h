#ifndef ZENCAPE_H_
#define ZENCAPE_H_

#define BUFFER_MAX 1024

void initiateMapper();

void *displayStart();

void setDisplay(int distance);

void *joystickStart();

int getSpeed();

#endif
