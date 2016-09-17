#ifndef axibram_h
#define axibram_h

#include <linux/ioctl.h>

struct bram_rw_data {
    uint32_t *data;
    uint32_t offset;
    uint32_t size;
};

#define TIMER_IOC_MAGIC  'b'

#define AXI_BRAM_READ            _IOR(TIMER_IOC_MAGIC, 1, struct bram_rw_data)
#define AXI_BRAM_WRITE           _IOW(TIMER_IOC_MAGIC, 2, struct bram_rw_data)

#endif
