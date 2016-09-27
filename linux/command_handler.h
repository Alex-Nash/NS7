#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include <inttypes.h>

#define MEM_OFFSET_DASHBOARD            (0x0000FF00)
#define MEM_OFFSET_POWER                (MEM_OFFSET_DASHBOARD + 0x08)
#define MEM_OFFSET_COMMAND_LEFT     	(MEM_OFFSET_DASHBOARD + 0x10)
#define MEM_OFFSET_COMMAND_RIGHT     	(MEM_OFFSET_DASHBOARD + 0x20)
//#define MEM_OFFSET_RIGHT_COMMAND     	(MEM_OFFSET_DASHBOARD + 0x06)

#define ENG_DIRECTION_REVERSE     0x00
#define ENG_DIRECTION_FORWARD     0x01

#define ENG_DISABLE               0x00
#define ENG_ENABLE                0x01

#define SMOOTHING		  5
#define MIN_DELAY         0x3F
#define MAX_DELAY         0x1FFFE
#define MIN_TORQ          0x0000
#define MAX_TORQ          0xFFFF

int SMOOTHING_FLAG;     //  1 - enable smoothing 0 - disable smoothing


struct move_command
{
  //uint16_t left_eng_direction;     // 0x01 - Forward ; 0x00 - Back
  int left_eng_speed;		       // 0 .. 100
  //uint32_t left_eng_delay;         // 0x3F .. 0x1FFFF
  //uint16_t right_eng_direction;    // 0x01 - Forward ; 0x00 - Back
  int right_eng_speed;		     // 0 .. 100
  //uint32_t right_eng_delay;        // 0x3F .. 0x1FFFF
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

uint32_t speed_to_delay(int speed);

uint32_t speed_to_torq(int speed);

int torq_to_speed(uint32_t torq, uint16_t direction);

int speed_smoothing (int cur_speed_value, int prev_speed_value);


int speed_smoothing (int cur_speed_value, int prev_speed_value);

int get_speed_value_from_ram(struct move_command *command);

int smoothing_move_command (struct move_command *cur_command);

#endif
