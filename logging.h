#ifndef APP_LOGGING_H
#define APP_LOGGING_H 

#include "mbed.h"

#ifdef DEBUG
static Serial pc(SERIAL_TX, SERIAL_RX);

#define LOG_INIT pc.baud(115200);wait(0.1);

#define LOG(msg, ...) pc.printf("\r\n" msg "\r\n", ##__VA_ARGS__)
#else
#define LOG(msg, ...)
#define LOG_INIT
#endif

#endif