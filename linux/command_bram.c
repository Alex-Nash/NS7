#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <linux/ioctl.h>

#include "command_bram.h"


int bram_memory_write(uint32_t offset, uint32_t *data, uint32_t length)
{
    int mem_file;
    mem_file = open(MEM_DEV, O_RDWR);

    if(mem_file < 0) {
        printf("AxiBram: can't open mem dev\n");
        return -1;
    }

    struct bram_rw_data rw_data;

    rw_data.size = length;
    rw_data.offset = (offset/4);
    rw_data.data = data;
    if(ioctl(mem_file, AXI_BRAM_WRITE, &rw_data)) {
        printf("AxiBram: error writting data\n");
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

    if(mem_file < 0) {
        printf("AxiBram: can't open mem dev\n");
        return -1;
    }

    struct bram_rw_data rw_data;

    rw_data.size = length;
    rw_data.offset = (offset >> 2);
    rw_data.data = malloc(length * BYTE_IN_CELL);

    if(ioctl(mem_file, AXI_BRAM_READ, &rw_data)) {
        printf("AxiBram: error writting data\n");
        close(mem_file);
        return -1;
    }

    memcpy(data, rw_data.data, length);
    free(rw_data.data);
    close(mem_file);

    return 0;
}
