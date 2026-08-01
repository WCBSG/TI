#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "zf_common_headfile.h"

#define FLASH_SIZE 32
extern int16 flash_buff[FLASH_SIZE];
extern int16 down_buff[FLASH_SIZE];

void config_save(void);
void config_load(void);
uint8 config_valid(void);

#endif