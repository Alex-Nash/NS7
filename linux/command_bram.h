#ifndef COMMAND_BRAM_H
#define COMMAND_BRAM_H

#include <inttypes.h>

#define MEM_DEV         "/dev/axi_bram"

struct bram_rw_data {
    uint32_t *data;
    uint32_t offset;
    uint32_t size;
};

#define TIMER_IOC_MAGIC  'b'

#define AXI_BRAM_READ            _IOR(TIMER_IOC_MAGIC, 1, struct bram_rw_data)
#define AXI_BRAM_WRITE           _IOW(TIMER_IOC_MAGIC, 2, struct bram_rw_data)


int bram_memory_write(uint32_t offset, uint32_t *data, uint32_t length);

#endif
