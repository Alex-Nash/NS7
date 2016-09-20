#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "command_bram.h"
#include "command_handler.h"

/*
*  Execute command
*  'p' - power enable/disable command 4 byte.
*           'ENA' - enable engines; 'DIS' - disable engine.
*        Example: 'pENA'
*  'm' - move command 8 byte. [left_eng_dir][left_eng_torq][right_eng_dir][right_eng_torq]
*          torq = 0..100;
*          '-' - move back ; '+' - move forward.
*        Example: 'm-100+050'
*/
int execute_command (char *command_str)
{
  char str;
  memcpy(&str, command_str, 1);


  switch (str) {
    case 'p':
      char power_cmd_str[3];
      memcpy(&power_cmd_str, &command_str[2], 3);
      execute_power_cmd(power_cmd_str);
      break;
    case 'm':
      char move_cmd_str[8];
      memcpy(&move_cmd_str, &command_str[2], 8);
      execute_move_cmd(move_cmd_str);
      break;
    default:
      printf ("Wrong command!\n");
      return -1;
  }

}

/*
* Write command to ram
* Return
*/

int execute_move_cmd(char *command_str)
{
  uint32_t mem_command[2];
  uint32_t torq, dir;
  int status;

  struct move_command cur_command;

  // parse move command
  status = parse_move_command(command_str, &cur_command);
  if (status == -1)
  {
    printf("Error parse move command! \n");
    return -1;
  }

  // *** Left engine command ***
  // define direction on command bite
  dir = (((uint32_t)(cur_command->left_eng_direction))<<16) & ((uint32_t)0xFFFF0000);
  // define torq on command bite
  torq = ((uint32_t)(cur_command->left_eng_torq)) & ((uint32_t)0x0000FFFF);
  // create 32 bit command
  mem_command[0] = (uint32_t)(torq | dir);

  // *** Right engine command ***
  // define direction on command bite
  dir = (((uint32_t)(cur_command->right_eng_direction))<<16) & ((uint32_t)0xFFFF0000);
  // define torq on command bite
  torq = ((uint32_t)(cur_command->right_eng_torq)) & ((uint32_t)0x0000FFFF);
  // create 32 bit command
  mem_command[1] = (uint32_t)(torq | dir);

  status = bram_memory_write((uint32_t)MEM_OFFSET_COMMAND , mem_command, 2);
  if ( status == -1)
  {
    printf("Error sending command\n");
    return -1;
  }

  return 0;
}

int parse_move_command(char *str, struct command *cur_command)
{
  uint16_t tmp_torq;

  // get left engine direction and set command struct
  if (str[1] == '-' || str[1] == '+')
  {
    if (str[1] == '-')
      cur_command->left_eng_direction = (uint16_t)(ENG_DIRECTION_REVERSE);
    else
      cur_command->left_eng_direction = (uint16_t)(ENG_DIRECTION_FORWARD);
  }
  else
    return -1;

  // get right engine direction and set command struct
  if (str[5] == '-' || str[5] == '+')
  {
    if (str[5] == '-')
      cur_command->left_eng_direction = (uint16_t)(ENG_DIRECTION_REVERSE);
    else
      cur_command->left_eng_direction = (uint16_t)(ENG_DIRECTION_FORWARD);
  }
  else
    return -1;


  char torq_buff[3];

  // get left engine TORQ and set command struct
  memcpy(torq_buff, &str[2], 3);
  tmp_torq = atoi(torq_buff);
  if (tmp_torq < 0 || tmp_torq > 100)
    return -1;

  cur_command->left_eng_torq = (uint16_t)(0xFFFF * tmp_torq/100.0);

  // get right engine TORQ and set command struct
  memcpy(torq_buff, &str[6], 3);
  tmp_torq = atoi(torq_buff);
  if (tmp_torq < 0 || tmp_torq > 100)
    return -1;

  cur_command->right_eng_torq = (uint16_t)(0xFFFF * tmp_torq/100.0);


  return 0;
}

int execute_power_cmd(char *command_str)
{
  uint32_t mem_command;
  if (strncmp(command_str, "ENA") == 0 || strncmp(command_str, "DIS") == 0)
  {
    if (strncmp(command_str, "ENA") == 0)
      mem_command = ENG_ENABLE;
    else
      mem_command = ENG_DISABLE;
  }
  else
  {
    printf("Error parse power command! \n");
    return -1;
  }



  status = bram_memory_write((uint32_t)MEM_OFFSET_POWER , mem_command, 1);
  if ( status == -1)
  {
    printf("Error sending command\n");
    return -1;
  }

  return 0;
}
