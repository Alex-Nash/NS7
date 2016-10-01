#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <inttypes.h>
#include <sys/stat.h>

#define DEBUG       3

#include "log.h"
#include "command_bram.h"

#define MEM_OFFSET 0x00000000

int file_loader(char *filename)
{
    int src_file;
    void *src_ptr;
    struct stat statbuf;

    if((src_file = open(filename, O_RDONLY)) < 0 )
    {
        ERROR_MSG("file loader: fail to open file (%s)", filename);
        return -1;
    }

    if(fstat(src_file, &statbuf) < 0 )
    {
        ERROR_MSG("file loader: fail to obtain file size");
        close(src_file);
        return -1;
    }

    if((src_ptr = mmap(0, statbuf.st_size, PROT_READ, MAP_SHARED, src_file, 0)) == MAP_FAILED )
    {
        ERROR_MSG("file loader: fail to map file");
        close(src_file);
        return -1;
    }

    if(bram_memory_write((uint32_t) MEM_OFFSET, src_ptr, (uint32_t)(statbuf.st_size / 4)) == -1)
    {
        ERROR_MSG("file loader: fail to write file");
        close(src_file);
        return -1;
    }

    close(src_file);

    return 0;
}
