#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mb_platform_init.h"

#include "bin_file_loader.c"
#include "command_bram.c"
#include "cos.c"
#include "gpio.c"

int main(int argc, char *argv[])
{
  // MB GPIO reset disable
  if (mb_stop() < 0) {
      DEBUG_PRINT("reset: error disable GPIO\n");
  }
  DEBUG_PRINT("reset: ENABLE!\n");
    // Set cos array
    if (set_cos_array() < 0) {
        DEBUG_PRINT("set_cos_array: error set cos array\n");
    }
    DEBUG_PRINT("set_cos_array: OK!\n");

    // Load bin file to the memmory
    if (file_loader("/home/mb_hello.bin") < 0) {
        DEBUG_PRINT("file_loader: error load file\n");
    }
    DEBUG_PRINT("file_loader: OK!\n");

    // MB GPIO reset enable
    if (mb_start() < 0) {
        DEBUG_PRINT("reset: error enable GPIO\n");
    }
    DEBUG_PRINT("reset: ENABLE!\n");

    return 0;
}
