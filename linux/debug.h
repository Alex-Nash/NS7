#ifndef DEBUG_H
#define DEBUG_H

#include "log.h"

#if defined(DEBUG) && DEBUG > 2
    #define DEBUG_MSG(fmt, args...) log("[%d:%s] " fmt "\n", \
        __LINE__, __func__, ##args)
#else
    #define DEBUG_MSG(fmt, args...)
#endif

#if defined(DEBUG) && DEBUG > 1
    #define ERROR_MSG(fmt, args...) log("[%d:%s] " fmt "\n", \
        __LINE__, __func__, ##args)
#else
    #define ERROR_MSG(fmt, args...)
#endif

#if defined(DEBUG) && DEBUG > 0
    #define USER_MSG(fmt, args...) log("  " fmt "\n", ##args)
#else
    #define USER_MSG(fmt, args...)
#endif

#endif // DEBUG_H
