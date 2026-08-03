/* IRPHOTO.h — 八路红外循迹：GPIO 读取 + 加权偏差 */
#ifndef __IRPHOTO_H__
#define __IRPHOTO_H__

#include "zf_common_headfile.h"

void IRPHOTO_Init(void);          /* 8 路 GPIO 输入（上拉） */
void IRPHOTO_Read(int s[8]);      /* 读状态：黑=1 白=0 */
int  calc_error(int s[8]);        /* 加权偏差：正值偏右，负值偏左 */

#endif
