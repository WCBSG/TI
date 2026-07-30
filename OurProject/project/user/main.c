#include "zf_common_headfile.h"
#include "camera.h"

#define WIFI_SSID_TEST      "1526"
#define WIFI_PASSWORD_TEST  "15261526"

void main(void)
{
    uint8 ball_x, ball_y;
    uint8 found;

    clock_init(SYSTEM_CLOCK_96M);
    debug_init();

    while(wifi_spi_init(WIFI_SSID_TEST, WIFI_PASSWORD_TEST))
    {
        system_delay_ms(100);
    }

    if(1 != WIFI_SPI_AUTO_CONNECT)
    {
        while(wifi_spi_socket_connect("TCP", WIFI_SPI_TARGET_IP, WIFI_SPI_TARGET_PORT, WIFI_SPI_LOCAL_PORT))
        {
            system_delay_ms(100);
        }
    }

    while(mt9v03x_init())
    {
        system_delay_ms(100);
    }

    seekfree_assistant_interface_init(SEEKFREE_ASSISTANT_WIFI_SPI);
    seekfree_assistant_camera_information_config(
        SEEKFREE_ASSISTANT_MT9V03X, camera_crop[0], CROP_MAX_COLS, CROP_MAX_ROWS);

    
    seekfree_assistant_camera_boundary_config(
        XY_BOUNDARY, 1, &ball_x, NULL, NULL, &ball_y, NULL, NULL);

    while(1)
    {
        camera_copy_image();
        camera_crop_image(50, 20, 0, 188);

        found = find_ball(&ball_x, &ball_y);
        if (!found)
        {
            ball_x = 0;
            ball_y = 0;
        }

        seekfree_assistant_camera_send();
    }
}
