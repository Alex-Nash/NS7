#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

#include "gpio.h"

int mb_start()
{
  int status;
  status = init_gpio(GPIO_MB_RESET_PIN);
  if (status  < 0)
  {
    printf("Failed to MB GPIO reset enable\n");
    return -1;
  }

  status = set_gpio_to_hi(GPIO_MB_RESET_PIN);
  if (status  < 0)
  {
    printf("Failed to MB GPIO reset enable\n");
    return -1;
  }

  status = close_gpio(GPIO_MB_RESET_PIN);
  if (status  < 0)
  {
    printf("Failed to MB GPIO reset enable\n");
    return -1;
  }

  return 0;
}

int mb_stop()
{
  int status;
  status = init_gpio(GPIO_MB_RESET_PIN);
  if (status  < 0)
  {
    printf("Failed to MB GPIO reset disable\n");
    return -1;
  }

  status = set_gpio_to_low(GPIO_MB_RESET_PIN);
  if (status  < 0)
  {
    printf("Failed to MB GPIO reset disable\n");
    return -1;
  }

  status = close_gpio(GPIO_MB_RESET_PIN);
  if (status  < 0)
  {
    printf("Failed to MB GPIO reset enable\n");
    return -1;
  }

  return 0;
}



int set_gpio_to_hi(uint16_t gpio_pin)
{
  int status, gpio_val;
  char gpio_str[4];

  sprintf(gpio_str, "%d", gpio_pin);

  status = init_gpio(gpio_pin);
  if (status < 0)
    return -1;

  char set_value_str[35];
  strcpy(set_value_str, "/sys/class/gpio/gpio");
  strcat(set_value_str, gpio_str);
  strcat(set_value_str, "/value");
  gpio_val = open(set_value_str, O_WRONLY);
  if (gpio_val < 0)
  {
    printf("Failed to open GPIO\n");
    return -1;
  }
  write(gpio_val, "1", 1);
  close(gpio_val);

  return 0;
}

int set_gpio_to_low(uint16_t gpio_pin)
{
  int status, gpio_val;
  char gpio_str[4];

  sprintf(gpio_str, "%d", gpio_pin);

  status = init_gpio(gpio_pin);
  if (status < 0)
    return -1;

  char set_value_str[35];
  strcpy(set_value_str, "/sys/class/gpio/gpio");
  strcat(set_value_str, gpio_str);
  strcat(set_value_str, "/value");
  gpio_val = open(set_value_str, O_WRONLY);
  if (gpio_val < 0)
  {
    printf("Failed to open GPIO\n");
    return -1;
  }
  write(gpio_val, "0", 1);
  close(gpio_val);

  return 0;
}

int init_gpio (uint16_t gpio_pin)
{
  int gpio_exp, gpio_direction;
  char gpio_str[4];

  sprintf(gpio_str, "%d", gpio_pin);

  gpio_exp = open("/sys/class/gpio/export", O_WRONLY);
  if (gpio_exp < 0)
  {
    printf("Failed to open GPIO to export\n");
    return -1;
  }
  write(gpio_exp, gpio_str, strlen(gpio_str));
  close(gpio_exp);

  char direction_str[35];
  strcpy(direction_str, "/sys/class/gpio/gpio");
  strcat(direction_str, gpio_str);
  strcat(direction_str, "/direction");
  gpio_direction = open(direction_str, O_WRONLY);
  if (gpio_direction < 0)
  {
    printf("Failed to open GPIO\n");
    return -1;
  }
  write(gpio_direction, "out", 3);
  close(gpio_direction);

  return 0;
}

int close_gpio (uint16_t gpio_pin)
{
  int gpio_unexp;
  char gpio_str[4];

  sprintf(gpio_str, "%d", gpio_pin);

  gpio_unexp = open("/sys/class/gpio/unexport", O_WRONLY);
  if (gpio_unexp < 0)
  {
    printf("Failed to open GPIO to unexport\n");
    return -1;
  }
  write(gpio_unexp, gpio_str, strlen(gpio_str));
  close(gpio_unexp);

  return 0;
}
