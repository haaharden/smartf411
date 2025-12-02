/**
 * ************************************************************************
 * 
 * @file MAX30102.c
 * @author zxr
 * @brief 
 * 
 * ************************************************************************
 * @copyright Copyright (c) 2024 zxr 
 * ************************************************************************
 */
#include "lcd.h"
#include "spi.h"
#include "gpio.h"
#include "font8x16.h"
#include "lcdfont.h"
#include "stdio.h"

/*SCK<->PA5
	SDA<->PA7
	CS<->PB0
	RS/DC<->PB1
	RESET<->PB2
	*/

/*================== 根据你的实际引脚名字改这里 ==================*/
/* 下面这些宏依赖 CubeMX 生成的 main.h 里有：
 * #define LCD_CS_Pin        GPIO_PIN_X
 * #define LCD_CS_GPIO_Port  GPIOB
 * ...
 * 如果你在 CubeMX 里用的不是这些名字，就把 LCD_CS_* 换成你自己的。
 */

#define TFT_CS_LOW()    HAL_GPIO_WritePin(LCD_CS_GPIO_Port,  LCD_CS_Pin,  GPIO_PIN_RESET)
#define TFT_CS_HIGH()   HAL_GPIO_WritePin(LCD_CS_GPIO_Port,  LCD_CS_Pin,  GPIO_PIN_SET)

#define TFT_DC_CMD()    HAL_GPIO_WritePin(LCD_DC_GPIO_Port,  LCD_DC_Pin,  GPIO_PIN_RESET)
#define TFT_DC_DATA()   HAL_GPIO_WritePin(LCD_DC_GPIO_Port,  LCD_DC_Pin,  GPIO_PIN_SET)

#define TFT_RST_LOW()   HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_RESET)
#define TFT_RST_HIGH()  HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_SET)

/* 如果你有单独的背光引脚，可以加一个宏；没有就忽略 */
// #define TFT_BL_ON()     HAL_GPIO_WritePin(LCD_BL_GPIO_Port, LCD_BL_Pin, GPIO_PIN_SET)
// #define TFT_BL_OFF()    HAL_GPIO_WritePin(LCD_BL_GPIO_Port, LCD_BL_Pin, GPIO_PIN_RESET)

/* 使用哪路 SPI：改成你实际用到的句柄 */
extern SPI_HandleTypeDef hspi1;
#define TFT_SPI  hspi1

/*================== 内部基础函数 ==================*/

/* 发送一个命令 */
static void tft_write_cmd(uint8_t cmd)
{
    TFT_CS_LOW();
    TFT_DC_CMD();
    HAL_SPI_Transmit(&TFT_SPI, &cmd, 1, HAL_MAX_DELAY);
    TFT_CS_HIGH();
}

/* 发送一串数据 */
//有些低效，之后肯定是要在外面通义片选和dc，不用没传一次数据就拉一次
static void tft_write_data(const uint8_t *data, uint16_t size)
{
    TFT_CS_LOW();
    TFT_DC_DATA();
    HAL_SPI_Transmit(&TFT_SPI, (uint8_t *)data, size, HAL_MAX_DELAY);
    TFT_CS_HIGH();
}

/* 发送一个 8bit 数据 */
static void tft_write_data8(uint8_t data)
{
    tft_write_data(&data, 1);
}

/* 发送一个 16bit 数据（RGB565） */
static void tft_write_data16(uint16_t data)
{
    uint8_t buf[2];
    buf[0] = data >> 8;      // 高字节在前（big endian）
    buf[1] = data & 0xFF;
    tft_write_data(buf, 2);
}

/* 设置窗口区域（带 offset） */
static void tft_set_addr_window(uint16_t x0, uint16_t y0,
                                uint16_t x1, uint16_t y1)
{
    uint8_t buf[4];

    x0 += TFT_X_OFFSET;
    x1 += TFT_X_OFFSET;
    y0 += TFT_Y_OFFSET;
    y1 += TFT_Y_OFFSET;

    /* 列地址 (Column) */
    tft_write_cmd(0x2A);  // CASET
    buf[0] = x0 >> 8;
    buf[1] = x0 & 0xFF;
    buf[2] = x1 >> 8;
    buf[3] = x1 & 0xFF;
    tft_write_data(buf, 4);

    /* 行地址 (Row) */
    tft_write_cmd(0x2B);  // RASET
    buf[0] = y0 >> 8;
    buf[1] = y0 & 0xFF;
    buf[2] = y1 >> 8;
    buf[3] = y1 & 0xFF;
    tft_write_data(buf, 4);

    /* 内存写入 */
    tft_write_cmd(0x2C);  // RAMWR
}

/*================== ST7789 初始化 ==================*/

/*
 * 这是一个通用的 ST7789 初始化序列，
 * 能先把屏点亮，后面你可以根据模组提供的代码微调。
 */
void TFT_Init(void)
{
    /* 1. 硬件复位 */
    TFT_RST_LOW();
    HAL_Delay(20);
    TFT_RST_HIGH();
    HAL_Delay(120);

    /* 2. 退出睡眠模式 */
    tft_write_cmd(0x11);      // SLPOUT
    HAL_Delay(120);

    /* 3. 像素格式：16bit */
    tft_write_cmd(0x3A);      // COLMOD
    tft_write_data8(0x55);    // 16 bits/pixel

    /* 4. 显示方向 / 内存扫描控制
     * 0x00: 正常方向
     * 常见组合：0x00 / 0x60 / 0xA0 / 0xC0，根据你想要的旋转来改。
     */
    tft_write_cmd(0x36);      // MADCTL
    tft_write_data8(0x00);

    /* 5. 一些常用电源/显示设置（简化版，可以跑起来再微调） */

    tft_write_cmd(0x21);      // INVON, 颜色反置打开（有些模组需要 ON 才是正常）
    // 如果颜色看着反了，可以改用 0x20: INVOFF

    tft_write_cmd(0x13);      // NORON, Normal Display Mode On
    HAL_Delay(10);

    /* 6. 打开显示 */
    tft_write_cmd(0x29);      // DISPON
    HAL_Delay(20);

    /* 7. 可选：清屏为黑色 */
    TFT_FillColor(0x0000);
}

/*================== 基础绘图函数 ==================*/

void TFT_DrawPixel(uint16_t x, uint16_t y, uint16_t color)
{
    if (x >= TFT_WIDTH || y >= TFT_HEIGHT)
        return;

    // 把绘图窗口设置成 1x1 的小区域
    tft_set_addr_window(x, y, x, y);

		tft_write_data16(color);

}

/* 填充一个矩形区域
	 stm32存的低字节在前，而屏幕要的是高字节在前，注意 */
void TFT_FillRect(uint16_t x, uint16_t y,
                  uint16_t w, uint16_t h, uint16_t color)
{		//下面三个if，目的是超过屏幕的就不显示
    if (x >= TFT_WIDTH || y >= TFT_HEIGHT)
        return;

    if ((x + w) > TFT_WIDTH)
        w = TFT_WIDTH - x;
    if ((y + h) > TFT_HEIGHT)
        h = TFT_HEIGHT - y;
		
    tft_set_addr_window(x, y, x + w - 1, y + h - 1);//告诉屏幕我要在这里画图

    uint32_t total = (uint32_t)w * h;

    // 注意：这次用 uint8_t 缓冲区，每个像素2字节
    uint8_t buf[64 * 2];
    uint8_t hi = color >> 8;       // 高字节
    uint8_t lo = color & 0xFF;     // 低字节

    for (int i = 0; i < 64; i++)
    {
        buf[2*i]     = hi; // 先发高字节
        buf[2*i + 1] = lo; // 再发低字节
    }

    TFT_CS_LOW(); //选中屏幕
    TFT_DC_DATA();//dc=1，接下来发送到是数据，不是命令
		//chunk是要这次烧的像素，total是总的还剩的像素
    while (total > 0)
    {
        uint16_t chunk = (total > 64) ? 64 : total;
        HAL_SPI_Transmit(&TFT_SPI, buf, chunk * 2, HAL_MAX_DELAY);
        total -= chunk;
    }

    TFT_CS_HIGH();//取消片选
}

/* 整屏填充一种颜色 */
void TFT_FillColor(uint16_t color)
{
    TFT_FillRect(0, 0, TFT_WIDTH, TFT_HEIGHT, color);
}
//横线
void TFT_DrawHLine(uint16_t x, uint16_t y,
                   uint16_t w, uint16_t color)
{
    TFT_FillRect(x, y, w, 1, color);
}
//竖线
void TFT_DrawVLine(uint16_t x, uint16_t y,
                   uint16_t h, uint16_t color)
{
    TFT_FillRect(x, y, 1, h, color);
}
//下面两个画圆
static void draw_circle_points(int xc, int yc, int x, int y, uint16_t color)
{
    TFT_DrawPixel(xc + x, yc + y, color);
    TFT_DrawPixel(xc - x, yc + y, color);
    TFT_DrawPixel(xc + x, yc - y, color);
    TFT_DrawPixel(xc - x, yc - y, color);
    TFT_DrawPixel(xc + y, yc + x, color);
    TFT_DrawPixel(xc - y, yc + x, color);
    TFT_DrawPixel(xc + y, yc - x, color);
    TFT_DrawPixel(xc - y, yc - x, color);
}

void TFT_DrawCircle(int xc, int yc, int r, uint16_t color)
{
    int x = 0;
    int y = r;
    int d = 3 - 2 * r;

    while (x <= y)
    {
        draw_circle_points(xc, yc, x, y, color);
        if (d < 0)
            d = d + 4 * x + 6;
        else
        {
            d = d + 4 * (x - y) + 10;
            y--;
        }
        x++;
    }
}

//画空心矩形
void TFT_DrawRect(uint16_t x, uint16_t y,
                  uint16_t w, uint16_t h, uint16_t color)
{
    if (w == 0 || h == 0) return;

    // 上边
    TFT_DrawHLine(x, y, w, color);
    // 下边
    TFT_DrawHLine(x, y + h - 1, w, color);
    // 左边
    TFT_DrawVLine(x, y, h, color);
    // 右边
    TFT_DrawVLine(x + w - 1, y, h, color);
}

// ch: 普通 ASCII 字符，比如 'A'、'0' 等
//ASCII可打印字符是32-126，所以从32开始，126-32+1=95
void TFT_DrawChar(uint16_t x, uint16_t y,
                  char ch, uint16_t fg, uint16_t bg)
{
    if (ch < 32 || ch > 126) ch = '?';  // 不认识的就画 '?'

    const uint8_t *bitmap = ascii_1608[ch - 32];

    for (uint8_t row = 0; row < 16; row++)
    {
        uint8_t bits = bitmap[row];
        for (uint8_t col = 0; col < 8; col++)
        {
            uint16_t color =
                (bits & (1 << col)) ? fg : bg;

            TFT_DrawPixel(x + col, y + row, color);
        }
    }
}

//字符串
void TFT_DrawString(uint16_t x, uint16_t y,
                    const char *str,
                    uint16_t fg, uint16_t bg)
{
    while (*str)
    {
        TFT_DrawChar(x, y, *str, fg, bg);
        x += 8;      // 每个字符宽 8 像素
        str++;
    }
}
//测试显示ui
void UI_Home(void)
{
    // 背景
    TFT_FillColor(0x0000);

    // 顶部状态栏
    TFT_FillRect(0, 0, TFT_WIDTH, 20, 0x2104);
    TFT_DrawString(5, 4, "My Device", 0xFFFF, 0x2104);
    TFT_DrawString(180, 4, "12:34",    0xFFFF, 0x2104);

    // 中间数据框
    TFT_DrawRect(10, 30, TFT_WIDTH - 20, 80, 0xFFFF);
    TFT_DrawString(20, 40, "TEMP:", 0xFFE0, 0x0000);
    TFT_DrawString(20, 60, "25.3 C", 0xFFFF, 0x0000);

    // 底部两个按钮
    TFT_FillRect(20, 130, 80, 40, 0x07E0);
    TFT_DrawString(35, 140, "START", 0x0000, 0x07E0);

    TFT_FillRect(140, 130, 80, 40, 0xF800);
    TFT_DrawString(155, 140, "STOP", 0xFFFF, 0xF800);
}
void UI_UpdateTemp(float temp)
{
    char buf[16];
    sprintf(buf, "%2.1f C", temp);

    // 先擦掉旧的数字区域
    TFT_FillRect(20, 60, 100, 16, 0x0000);
    // 再画新的
    TFT_DrawString(20, 60, buf, 0xFFFF, 0x0000);
}
