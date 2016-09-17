#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>


#include "axi_bram.h"

#define MEM_DEV         "/dev/axi_bram"

#define BYTE_IN_CELL    4

void usage()
{
    printf("\nUsage: us-mem [-o <offset>] [-r <size>] [-w <value>]\n\n");
    printf("\t -o <offset> \t memory offset\n");
    printf("\t -r <num cells> read number of cells from memory\n");
    printf("\t -w <value> \t write value (in HEX between 0x00000000 and 0xffffffff) to memory\n");
}

int main(int argc, char *argv[])
{
    if(argc < 4) {
        printf("Invalid arguments\n");
        usage();
        return -1;
    }

    char *argopts = "o:r:w:";
    int arg;

    int offset = 0, num_of_cell = 0;

    int mem_file;

    mem_file = open(MEM_DEV, O_RDWR);

    if(mem_file < 0) {
        printf("can't open mem dev\n");
        return -1;
    }

    struct bram_rw_data rw_data;

    while((arg = getopt(argc, argv, argopts)) != -1) {
        switch (arg) {
        case 'o':
            offset = atoi(optarg);
            break;
        case 'r':
            num_of_cell = atoi(optarg);
            if(num_of_cell == 0) {
                printf("error size = 0\n");
                return -1;
            }
            rw_data.size = num_of_cell;
            rw_data.offset = offset;
            rw_data.data = malloc(num_of_cell * BYTE_IN_CELL);
            if(ioctl(mem_file, AXI_BRAM_READ, &rw_data)) {
                printf("error reading data\n");
                free(rw_data.data);
                return -1;
            }
            int i;
            for(i = 0; i < num_of_cell; i++) {
                printf ("0x%08x ", (uint32_t)*(rw_data.data+i));
            }
            break;
        case 'w':
            if(strlen(optarg) != 10) {
                printf("incorrect value\n");
                usage();
                return -1;
            }

            uint32_t hex_value;
            hex_value = strtoul(optarg, NULL, 16);
            rw_data.size = 1;
            rw_data.offset = offset;
            rw_data.data = &hex_value;
            if(ioctl(mem_file, AXI_BRAM_WRITE, &rw_data)) {
                printf("error writting data\n");
                return -1;
            }
            printf("DATA is writеen: 0x%08x\n", *(rw_data.data));
            break;
        default:
            usage();
            return -1;
        }
    }

    close(mem_file);
}
