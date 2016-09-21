#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include <inttypes.h>

#define MEM_OFFSET_DASHBOARD    0x8600
#define MEM_OFFSET_POWER        (MEM_OFFSET_DASHBOARD + 0x08)
#define MEM_OFFSET_COMMAND     	(MEM_OFFSET_DASHBOARD + 0x10)
//#define MEM_OFFSET_RIGHT_COMMAND     	(MEM_OFFSET_DASHBOARD + 0x06)

#define ENG_DIRECTION_REVERSE     0x00
#define ENG_DIRECTION_FORWARD     0x01

#define ENG_DISABLE               0x00
#define ENG_ENABLE                0x01

#define SMOOTHING		0xFFFFFF


struct move_command
{
  uint16_t left_eng_direction;     // 0x01 - Forward ; 0x00 - Back
  uint16_t left_eng_torq;		       // 0x0000..0xFFFF
  uint16_t right_eng_direction;    // 0x01 - Forward ; 0x00 - Back
  uint16_t right_eng_torq;		     // 0x0000..0xFFFF
  uint32_t left_eng_speed;
  uint32_t right_eng_speed;
};

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
int execute_command (char *command_str);

int execute_move_cmd(char *command_str);

int parse_move_command(char *str, struct move_command *cur_command);

int execute_power_cmd(char *command_str);


#endif
