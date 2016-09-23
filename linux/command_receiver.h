#ifndef COMMAND_RECEIVER_H
#define COMMAND_RECEIVER_H


#if defined(DEBUG)
 #define DEBUG_PRINT(fmt, args...) printf("DEBUG: %d:%s(): " fmt "\n", \
    __LINE__, __func__, ##args)
#else
 #define DEBUG_PRINT(fmt, args...)
#endif

#define CMDSIZE     12 // number of bytes for commands

/*
#define LEFT        0x10 // value for cmd: select left motor
#define RIGHT       0x11 // value for cmd: select right motor

#define START       0x1 // cmd1 for test
#define STOP        0x2 // cmd2 for test

#define REVERS      0x3

#define STOP_SERVER 0xFF // stop server   */

#endif // DEBUG_H
