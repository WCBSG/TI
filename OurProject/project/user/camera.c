#include "camera.h"

uint8 camera_copy[120][188];
uint8 camera_crop[CROP_MAX_ROWS][CROP_MAX_COLS];

void camera_copy_image(void)
{
    uint8 r, c;

    if (mt9v03x_finish_flag)
    {
        for (r = 0; r < 120; r++)
        {
            for (c = 0; c < 188; c++)
            {
                camera_copy[r][c] = mt9v03x_image[r][c];
            }
        }
        mt9v03x_finish_flag = 0;
    }
}

void camera_crop_image(uint8 row_start, uint8 rows, uint8 col_start, uint8 cols)
{
    uint8 r, c;

    for (r = 0; r < rows; r++)
    {
        for (c = 0; c < cols; c++)
        {
            camera_crop[r][c] = camera_copy[row_start + r][col_start + c];
        }
    }
}

// ---- 钢球检测（大津法二值化）----
// 特征：高亮中心 + 黑色圆环
// 返回值：1=找到, 0=未找到  *x=列坐标, *y=行坐标

#define  BALL_RING_RADIUS       4
#define  BALL_RING_MIN_DARK     5
#define  BALL_MIN_BRIGHT        150     // 高亮点至少这个亮度

static uint8 otsu_threshold(uint8 *img, uint16 total)
{
    uint32 hist[256];
    uint16 i;
    uint32 sum_all, sum_b, w_b, w_f, max_var, var;
    uint8  t, best_t;

    for (i = 0; i < 256; i++) hist[i] = 0;
    for (i = 0; i < total; i++) hist[img[i]]++;

    sum_all = 0;
    for (i = 0; i < 256; i++)
        sum_all += (uint32)i * hist[i];

    w_b    = 0;
    sum_b  = 0;
    max_var = 0;
    best_t  = 128;

    for (t = 0; t < 255; t++)
    {
        w_b += hist[t];
        if (w_b == 0) continue;
        w_f = total - w_b;
        if (w_f == 0) break;
        sum_b += (uint32)t * hist[t];

        var = w_b * w_f * (sum_b / w_b - (sum_all - sum_b) / w_f)
            * (sum_b / w_b - (sum_all - sum_b) / w_f);

        if (var > max_var)
        {
            max_var = var;
            best_t  = t;
        }
    }
    return best_t;
}

uint8 find_ball(uint8 *x, uint8 *y)
{
    uint8  r, c, i, cnt;
    uint8  max_val, max_r, max_c;
    uint8  thresh;
    int16  rr, cc;
    int8   dirs[8][2] = {
        { 0, 4}, { 3, 3}, { 4, 0}, { 3,-3},
        { 0,-4}, {-3,-3}, {-4, 0}, {-3, 3}
    };

    // 大津法求阈值
    thresh = otsu_threshold(&camera_crop[0][0], CROP_MAX_ROWS * CROP_MAX_COLS);

    // 找最亮点（高光）
    max_val = 0;
    max_r   = 0;
    max_c   = 0;
    for (r = 0; r < CROP_MAX_ROWS; r++)
    {
        for (c = 0; c < CROP_MAX_COLS; c++)
        {
            if (camera_crop[r][c] > max_val)
            {
                max_val = camera_crop[r][c];
                max_r   = r;
                max_c   = c;
            }
        }
    }

    if (max_val < BALL_MIN_BRIGHT)
        return 0;

    // 8 方向采样暗环
    cnt = 0;
    for (i = 0; i < 8; i++)
    {
        rr = (int16)max_r + dirs[i][0];
        cc = (int16)max_c + dirs[i][1];
        if (rr >= 0 && rr < CROP_MAX_ROWS && cc >= 0 && cc < CROP_MAX_COLS)
        {
            if (camera_crop[rr][cc] <= thresh)
                cnt++;
        }
    }

    if (cnt < BALL_RING_MIN_DARK)
        return 0;

    *x = max_c;
    *y = max_r;
    return 1;
}
