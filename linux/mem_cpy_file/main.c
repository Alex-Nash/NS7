#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <inttypes.h>

//#define offset_addr    0x30000000

void usage()
{
    printf("\nUsage: cpy_file [<file name>] [<memmory offset>]\n");
    return;
}

int main(int argc, char *argv[])
{
    if(argc != 3) {
        usage();
        return -1;
    }

    int src_file;
    struct stat statbuf;

    if((src_file = open(argv[1], O_RDONLY)) < 0 ) {
        printf("can't open file %s for reading\n", argv[1]);
        return -1;
    }

    uint32_t offset_addr;
    offset_addr = strtoul(argv[2], NULL, 16);

    if(fstat(src_file, &statbuf) < 0 ) {
        printf("can't obtain file size\n");
        return -1;
    }

    void *src_ptr;

    if((src_ptr = mmap(0, statbuf.st_size, PROT_READ, MAP_SHARED, src_file, 0)) == MAP_FAILED ) {
        printf("failed to map file to the memory\n");
        return -1;
    }

    int mem_file;

    if((mem_file = open("/dev/mem", O_RDWR)) < 0 ) {
        printf("can't open memory file\n");
        return -1;
    }

    void *microblaze_ptr;

    if ((microblaze_ptr = mmap(0, statbuf.st_size, PROT_WRITE, MAP_SHARED, mem_file, offset_addr)) == MAP_FAILED ) {
        printf("failed to map microblaze memory\n");
        return -1;
    }

    memcpy(microblaze_ptr, src_ptr, statbuf.st_size);

    printf("File successfully written!\n");

    return 0;
}
