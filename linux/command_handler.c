#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "command_bram.h"
#include "command_handler.h"

/*
* Write command to ram
|           32 bit            |
|  LEFT ENGINE | RIGHT ENGINE |
|DIRECTION|TORQ|DIRECTION|TORQ|

* Return
*/

int send_command(struct command *cur_command)
{
  uint32_t mem_command[2];
  uint32_t torq, dir;
  int mem_cell = 0;

  mem_command[0] = 0;
  mem_command[1] = 0;

  if (cur_command->position == 'l')
  	mem_cell = 0;
  else
	  mem_cell = 1;

  // define direction on command bite
  dir = (((uint32_t)(cur_command->direction))<<16) & ((uint32_t)0xFFFF0000);
  // define torq on command bite
  torq = ((uint32_t)(cur_command->torq)) & ((uint32_t)0x0000FFFF);
  // create 32 bit command
  mem_command[mem_cell] = (uint32_t)(torq | dir);

  if (bram_memory_write((uint32_t)MEM_OFFSET_COMMAND , mem_command, 2) < 0)
    printf("Command: error sending command\n");
  return 0;
}

int parse_command(struct command *cur_command, char *str)
{
  uint16_t tmp_torq;

  if (str[0] == 'r' || str[0] == 'l')
    cur_command->position = (char)str[0];
  else
    return -1;

  if (str[2] == '-' || str[2] == '+')
  {
    if (str[2] == '-')
      cur_command->direction = (uint16_t)(ENG_DIRECTION_REVERSE);
    else
      cur_command->direction = (uint16_t)(ENG_DIRECTION_FORWARD);
  }
  else
    return -1;


  char torq_buff[4];
  memcpy(torq_buff, &str[3], 4);
  tmp_torq = atoi(torq_buff);
  if (tmp_torq < 0 || tmp_torq > 100)
    return -1;

  cur_command->torq = (uint16_t)(0xFFFF * tmp_torq/100.0);
  cur_command->enable = (uint16_t)ENG_ENABLE;

  return 0;
}
