#include "zf_common_headfile.h"
#include "control.h"
#include "camera.h"
#include "IRPHOTO.h"
#include "Motor.h"
#include "PID.h"


//#define WIFI_SSID_TEST      "1526"
//#define WIFI_PASSWORD_TEST  "15261526"

void main(void)
{
   // uint8 ball_x, ball_y;
   // uint8 found;
    int s[8];
    clock_init(SYSTEM_CLOCK_96M);
    debug_init();
    tft180_init();
    tft180_set_color(0x0000, 0xFFFF);  // 黑字白底
    tft180_clear(0xFFFF);              // 清屏为白色
    s[0] = 1;
    s[1] = 1;
    IRPHOTO_Init();
    IRPHOTO_Display(s);
    //while(wifi_spi_init(WIFI_SSID_TEST, WIFI_PASSWORD_TEST))
    //{
    //    system_delay_ms(100);
    //}

    //if(1 != WIFI_SPI_AUTO_CONNECT)
    //{
    //    while(wifi_spi_socket_connect("TCP", WIFI_SPI_TARGET_IP, WIFI_SPI_TARGET_PORT, WIFI_SPI_LOCAL_PORT))
    //     {
    //         system_delay_ms(100);
    //     }
    // }

    // while(mt9v03x_init())
    // {
    //     system_delay_ms(100);
    // }

    // seekfree_assistant_interface_init(SEEKFREE_ASSISTANT_WIFI_SPI);
    // seekfree_assistant_camera_information_config(
    //     SEEKFREE_ASSISTANT_MT9V03X, camera_crop[0], CROP_MAX_COLS, CROP_MAX_ROWS);

    
    // seekfree_assistant_camera_boundary_config(
    //     XY_BOUNDARY, 1, &ball_x, NULL, NULL, &ball_y, NULL, NULL);
    // s[0] = 1;
    // s[1] = 1;
    while(1)
    {
        // camera_copy_image();
        // camera_crop_image(50, 20, 0, 188);

        // found = find_ball(&ball_x, &ball_y);
        // if (!found)
        // {
        //     ball_x = 0;
        //     ball_y = 0;
        // }

        // seekfree_assistant_camera_send();
         uint8 val = (P3 >> 4) & 0x01;  // 直接读 P34
        tft180_show_uint8(10, 30, val);
        system_delay_ms(100);
    // IRPHOTO_Read(s);
    // IRPHOTO_Display(s);  // 屏幕上显示 8 个数字
    system_delay_ms(50);
    }
}
