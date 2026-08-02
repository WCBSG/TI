#ifndef __IRPHOTO_H__
#define __IRPHOTO_H__

#include "zf_common_headfile.h"

void IRPHOTO_Init(void);
void IRPHOTO_Read(int s[8]);
int  calc_error(int s[8]);

#endif
