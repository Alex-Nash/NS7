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
      printf ("execute_command: Wrong command!\n");
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
  uint32_t mem_command_left[2];
  uint32_t mem_command_right[2];
  uint32_t  prev_left_eng_speed, prev_right_eng_speed, speed_step;
  uint32_t torq, dir;
  int status;

  struct move_command cur_command;

  // parse move command
  status = parse_move_command(command_str, &cur_command);
  if (status == -1)
  {
    printf("execute_move_cmd: Error parse move command! \n");
    return -1;
  }

  // *** Left engine command ***
  // define direction on command bite
  if (cur_command.left_eng_speed >= 0)
    dir = (((uint32_t)(ENG_DIRECTION_FORWARD))<<16) & ((uint32_t)0xFFFF0000);
  else
    dir = (((uint32_t)(ENG_DIRECTION_REVERSE))<<16) & ((uint32_t)0xFFFF0000);

  // define torq on command bite
  torq = speed_to_torq(cur_command.left_eng_speed) & ((uint32_t)0x0000FFFF);
  // create 32 bit command
  mem_command_left[0] = (uint32_t)(torq | dir);
  // set left engine delay
  mem_command_left[1] = speed_to_delay(cur_command.left_eng_speed);

  status = bram_memory_write((uint32_t)MEM_OFFSET_COMMAND_LEFT , mem_command_left, 2);
  if ( status == -1)
  {
    printf("execute_move_cmd: Error sending command\n");
    return -1;
  }

  // *** Right engine command ***
  // define direction on command bite
  if (cur_command.right_eng_speed >= 0)
    dir = (((uint32_t)(ENG_DIRECTION_FORWARD))<<16) & ((uint32_t)0xFFFF0000);
  else
    dir = (((uint32_t)(ENG_DIRECTION_REVERSE))<<16) & ((uint32_t)0xFFFF0000);
  // define torq on command bite
  torq = speed_to_torq(cur_command.right_eng_speed ) & ((uint32_t)0x0000FFFF);
  // create 32 bit command
  mem_command_right[0] = (uint32_t)(torq | dir);
  // set right engine speed
  mem_command_right[1] = speed_to_delay(cur_command.right_eng_speed);

  status = bram_memory_write((uint32_t)MEM_OFFSET_COMMAND_RIGHT , mem_command, 2);
  if ( status == -1)
  {
    printf("execute_move_cmd: Error sending command\n");
    return -1;
  }


  return 0;
}

int parse_move_command(char *str, struct move_command *cur_command)
{
  int speed;
  int left_eng_direction, right_eng_direction;

  // get left engine direction and set command struct
  if (str[0] == '-' || str[0] == '+')
  {
    if (str[0] == '-')
      left_eng_direction = -1;
    else
      left_eng_direction = 1;
  }
  else
    return -1;

  // get right engine direction and set command struct
  if (str[4] == '-' || str[4] == '+')
  {
    if (str[4] == '-')
      right_eng_direction = -1;
    else
      right_eng_direction = 1;
  }
  else
    return -1;

  char speed_buff[3];

  // get left engine speed and set command struct
  memcpy(speed_buff, &str[1], 3);
  speed = atoi(speed_buff);
  if (speed < 0 || speed > 100)
    return -1;

  cur_command->left_eng_speed = left_eng_direction * speed;

  // get right engine speed and set command struct
  memcpy(speed_buff, &str[5], 3);
  speed = atoi(speed_buff);
  if (speed < 0 || speed > 100)
    return -1;

  cur_command->right_eng_speed = right_eng_direction * speed;

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
    printf("execute_power_cmd: Error parse power command! \n");
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

uint32_t speed_to_delay(uint16_t speed)
{
  uint32_t delay;

  if (speed > 100)
    speed = 100;
  if (speed < 0)
    speed = 0;

  delay = MAX_DELAY - ((MAX_DELAY - MIN_DELAY) * speed / 100.0);

  return delay;
}

uint32_t speed_to_torq(uint16_t speed)
{
  uint32_t torq;

  if (speed > 100)
    speed = 100;
  if (speed < 0)
    speed = 0;

  torq = MIN_TORQ + ((MAX_TORQ - MIN_TORQ) * speed / 100.0);
  if (speed == 0)
    torq = 0;

  return torq;

}

int torq_to_speed(uint32_t torq, uint16_t direction)
{
  int speed;

  if (torq > MAX_TORQ)
    speed = MAX_TORQ;
  if (torq < MIN_TORQ)
    speed = MIN_TORQ;

  speed = (torq - MIN_TORQ) * 100 / (MAX_TORQ - MIN_TORQ);
  if (direction == ENG_DIRECTION_REVERSE)
    return -speed;
  return speed;
}

int speed_smoothing (int cur_speed_value, int prev_speed_value)
{
  /*
  if(fabs(cur_speed_value - prev_speed_value) > SMOOTHING)
  {

  }
 */
  return 0;
}
