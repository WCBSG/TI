#ifndef __IRPHOTO_H__
#define __IRPHOTO_H__

#include "zf_common_headfile.h"

void IRPHOTO_Init(void);
void IRPHOTO_Read(int s[8]);
int  calc_error(int s[8]);
int  is_stop(int s[8]);
void IRPHOTO_Display(int s[8]);
void IRPHOTO_TestSync(void);   /* IR 测试页：实时绘制传感器状态 */

#endif
