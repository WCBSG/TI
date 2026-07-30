#ifndef __IRPHOTO_H__
#define __IRPHOTO_H__

#include "zf_common_headfile.h"

void IRPHOTO_Init(void);
int  calc_error(int s[8]);
int  is_stop(int s[8]);

#endif
