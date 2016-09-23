#ifndef MB_PLATFORM_INIT_H
#define MB_PLATFORM_INIT_H


#if defined(DEBUG)
 #define DEBUG_PRINT(fmt, args...) printf("DEBUG: %d:%s(): " fmt "\n", \
    __LINE__, __func__, ##args)
#else
 #define DEBUG_PRINT(fmt, args...)
#endif


#endif // DEBUG_H
