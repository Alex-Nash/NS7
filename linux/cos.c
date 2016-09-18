#include <math.h>
#include "command_bram.h"

#include "cos.h"

int set_cos_array ()
{
    double delta;
    uint32_t i;
    uint32_t cos_value[COS_ARRAY_LENGTH];

    delta = 2.0 * M_PI / COS_ARRAY_LENGTH;

    for (i=0; i < COS_ARRAY_LENGTH; i++)
    {
        cos_value[i] = (uint32_t)((0xFFFFFFFF)/2 * (cos(i * delta) + 1));
    }

    bram_memory_write((uint32_t) MEM_OFFSET_COS, cos_value, COS_ARRAY_LENGTH);

    return 0;
}
