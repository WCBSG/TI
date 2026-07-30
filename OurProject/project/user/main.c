#include "zf_common_headfile.h"
#include "WcTFT180.h"
#include "tft_test.h"


void main(void)
{
    clock_init(SYSTEM_CLOCK_96M);
    debug_init();
    WcTFT_Init();       /* 安全屏幕接口初始化（内部调用 tft180_init + 光标初始化） */

    tft_test_run();     /* 循环执行 6 项屏幕功能测试 */
}
