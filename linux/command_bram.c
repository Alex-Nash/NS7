#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/ioctl.h>
#include <memory.h>

#define DEBUG       3

#include "log.h"
#include "command_bram.h"


int bram_memory_write(uint32_t offset, uint32_t *data, uint32_t length)
{
    int mem_file;
    mem_file = open(MEM_DEV, O_RDWR);

    if(mem_file < 0)
    {
        ERROR_MSG("axi bram: fail to open");
        return -1;
    }

    struct bram_rw_data rw_data;

    rw_data.size = length;
    rw_data.offset = (offset/4); // convert offset from 8 bit to 32 bit
    rw_data.data = data;
    if(ioctl(mem_file, AXI_BRAM_WRITE, &rw_data))
    {
        ERROR_MSG("axi bram: fail to write");
        close(mem_file);
        return -1;
    }

    close(mem_file);

    return 0;
}

int bram_memory_read(uint32_t offset, uint32_t *data, uint32_t length)
{
    int mem_file;
    mem_file = open(MEM_DEV, O_RDWR);

    if(mem_file < 0)
    {
        ERROR_MSG("axi bram: fail to open");
        return -1;
    }

    struct bram_rw_data rw_data;

    rw_data.size = length;
    rw_data.offset = (offset/4); // convert offset from 8 bit to 32 bit
    rw_data.data = malloc(length * BYTE_IN_CELL);

    if(ioctl(mem_file, AXI_BRAM_READ, &rw_data))
    {
        ERROR_MSG("axi bram: fail to write");
        close(mem_file);
        return -1;
    }

    memcpy(data, rw_data.data, length * 4);
    free(rw_data.data);
    close(mem_file);

    return 0;
}
