#ifndef DEBUG_H
#define DEBUG_H

#if defined(DEBUG) && DEBUG > 0
 #define DEBUG_PRINT(fmt, args...) printk(KERN_ALERT "DEBUG: %d:%s(): " fmt "\n", \
    __LINE__, __func__, ##args)
#else
 #define DEBUG_PRINT(fmt, args...)
#endif

#endif // DEBUG_H
