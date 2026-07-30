#ifndef __CAMERA_H__
#define __CAMERA_H__

#include "zf_common_headfile.h"

#define CROP_MAX_ROWS  20
#define CROP_MAX_COLS  188

extern uint8 camera_copy[120][188];
extern uint8 camera_crop[CROP_MAX_ROWS][CROP_MAX_COLS];

void camera_copy_image(void);
void camera_crop_image(uint8 row_start, uint8 rows, uint8 col_start, uint8 cols);

uint8 find_ball(uint8 *x, uint8 *y);

#endif
