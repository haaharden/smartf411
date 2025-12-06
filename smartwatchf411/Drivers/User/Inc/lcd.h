/**
 * ************************************************************************
 * 
 * @file LCD.h
 * @author zxr
 * @brief 
 * 
 * ************************************************************************
 * @copyright Copyright (c) 2024 zxr 
 * ************************************************************************
 */
#ifndef _LCD_H
#define _LCD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "lvgl.h"
	
/*================ 基本参数，根据你的屏幕实际情况修改 =================*/

/* 分辨率：先按 240x280 写，如果你的是 240x240 自己改这里 */
#define TFT_WIDTH   240
#define TFT_HEIGHT  280

/* 如果你的屏幕需要偏移（常见 240x280 面板会有 X/Y offset），可以在这里改 */
#define TFT_X_OFFSET   0
#define TFT_Y_OFFSET   20

/*================ 对外 API =================*/

void tft_set_addr_window(uint16_t x0, uint16_t y0,uint16_t x1, uint16_t y1);
	
void TFT_Init(void);

void tft_write_data_dma(uint8_t *data, uint16_t size);

/* 整屏填充一种颜色（RGB565） */
void TFT_FillColor(uint16_t color);

/* 填充一个矩形区域（左上角 x,y，宽 w，高 h） */
void TFT_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

/* 画一个像素 */
void TFT_DrawPixel(uint16_t x, uint16_t y, uint16_t color);

//横线
void TFT_DrawHLine(uint16_t x, uint16_t y,uint16_t w, uint16_t color);

//竖线
void TFT_DrawVLine(uint16_t x, uint16_t y,uint16_t h, uint16_t color);

//画圆
void TFT_DrawCircle(int xc, int yc, int r, uint16_t color);

//画空心矩形
void TFT_DrawRect(uint16_t x, uint16_t y,uint16_t w, uint16_t h, uint16_t color);

//显示字符串
void TFT_DrawString(uint16_t x, uint16_t y,const char *str,uint16_t fg, uint16_t bg);

//UI函数
void TFT_UI_Home(void);
void TFT_UI_UpdateTemp(float temp);

void TFT_FlushArea(uint16_t x1, uint16_t y1,uint16_t x2, uint16_t y2,const lv_color_t *color_p);
#ifdef __cplusplus
}
#endif

#endif


