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
  char power_cmd_str[3];
  char move_cmd_str[8];

  switch (str) {
    case 'p':
      
      memcpy(power_cmd_str, &(command_str[1]), 3);
      execute_power_cmd(power_cmd_str);
      break;
    case 'm':
      
      memcpy(move_cmd_str, &(command_str[1]), 8);
      execute_move_cmd(move_cmd_str);
      break;
    default:
      printf ("Wrong command!\n");
      return -1;
  }

  return 0;

}

/*
* Write command to ram
* Return
*/

int execute_move_cmd(char *command_str)
{
  uint32_t mem_command[4];
  uint32_t prev_mem_command[4];
  uint32_t  prev_left_eng_speed, prev_right_eng_speed, speed_step;
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
  
  status = bram_memory_read((uint32_t)MEM_OFFSET_COMMAND , prev_mem_command, 4);
  if ( status == -1)
  {
    printf("Error sending command\n");
    return -1;
  }

  //smoothing alg
/*
  prev_left_eng_speed = 0xFFFFFFFF - prev_mem_command[1];
  speed_step = (prev_left_eng_speed > cur_command.left_eng_speed) ? 
		prev_left_eng_speed - cur_command.left_eng_speed : 
		cur_command.left_eng_speed - prev_left_eng_speed;
  
  if(speed_step > SMOOTHING && chenge_dir == 0 )
  {
     cur_command.left_eng_speed = (prev_left_eng_speed > cur_command.left_eng_speed) ? 
		                   prev_left_eng_speed - SMOOTHING : 
		                   SMOOTHING - prev_left_eng_speed;
  
  }
*/


  // *** Left engine command ***
  // define direction on command bite
  dir = (((uint32_t)(cur_command.left_eng_direction))<<16) & ((uint32_t)0xFFFF0000);

  

  // define torq on command bite

  torq = ((uint32_t)(cur_command.left_eng_speed >> 16)) & ((uint32_t)0x0000FFFF);
  // create 32 bit command
  mem_command[0] = (uint32_t)(torq | dir);
  // set left engine speed
  mem_command[1] = (uint32_t)(0xFFFFFFFF - cur_command.left_eng_speed);

  // *** Right engine command ***
  // define direction on command bite
  dir = (((uint32_t)(cur_command.right_eng_direction))<<16) & ((uint32_t)0xFFFF0000);
  // define torq on command bite
  torq = ((uint32_t)(cur_command.right_eng_speed >> 16)) & ((uint32_t)0x0000FFFF);
  // create 32 bit command
  mem_command[2] = (uint32_t)(torq | dir);
  // set right engine speed
  mem_command[3] = (uint32_t)(0xFFFFFFFF - cur_command.right_eng_speed);

  status = bram_memory_write((uint32_t)MEM_OFFSET_COMMAND , mem_command, 4);
  if ( status == -1)
  {
    printf("Error sending command\n");
    return -1;
  }


  return 0;
}

int parse_move_command(char *str, struct move_command *cur_command)
{
  uint16_t tmp_torq;

  // get left engine direction and set command struct
  if (str[0] == '-' || str[0] == '+')
  {
    if (str[0] == '-')
      cur_command->left_eng_direction = (uint16_t)(ENG_DIRECTION_REVERSE);
    else
      cur_command->left_eng_direction = (uint16_t)(ENG_DIRECTION_FORWARD);
  }
  else
    return -1;

  // get right engine direction and set command struct
  if (str[4] == '-' || str[4] == '+')
  {
    if (str[4] == '-')
      cur_command->right_eng_direction = (uint16_t)(ENG_DIRECTION_REVERSE);
    else
      cur_command->right_eng_direction = (uint16_t)(ENG_DIRECTION_FORWARD);
  }
  else
    return -1;


  char torq_buff[3];

  // get left engine TORQ and set command struct
  memcpy(torq_buff, &str[1], 3);
  tmp_torq = atoi(torq_buff);
  if (tmp_torq < 0 || tmp_torq > 100)
    return -1;

  cur_command->left_eng_speed = (uint32_t)(0xFFFFFFFF * tmp_torq/100.0);
  cur_command->left_eng_torq = (uint16_t)(cur_command->left_eng_speed >> 16);

  // get right engine TORQ and set command struct
  memcpy(torq_buff, &str[5], 3);
  tmp_torq = atoi(torq_buff);
  if (tmp_torq < 0 || tmp_torq > 100)
    return -1;

  cur_command->right_eng_speed = (uint32_t)(0xFFFFFFFF * tmp_torq/100.0);
  cur_command->right_eng_torq = (uint16_t)(cur_command->left_eng_speed >> 16);


  return 0;
}

int execute_power_cmd(char *command_str)
{
  int status;
  uint32_t mem_command;
  if (strncmp(command_str, "ENA", 3) == 0 || strncmp(command_str, "DIS", 3) == 0)
  {
    if (strncmp(command_str, "ENA", 3) == 0)
      mem_command = ENG_ENABLE;
    else
      mem_command = ENG_DISABLE;
  }
  else
  {
    printf("Error parse power command! \n");
    return -1;
  }



  status = bram_memory_write((uint32_t)MEM_OFFSET_POWER , &mem_command, 1);
  if ( status == -1)
  {
    printf("Error sending command\n");
    return -1;
  }

  return 0;
}
