/**
 * @file lv_port_indev_templ.c
 *
 */

/*Copy this file as "lv_port_indev.c" and set this value to "1" to enable content*/
#if 1

/*********************
 *      INCLUDES
 *********************/
#include "lv_port_indev.h"
#include "lvgl.h"
#include "touch.h"
/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void touchpad_init(void);
static void touchpad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data);
static bool touchpad_is_pressed(void);
static void touchpad_get_xy(lv_coord_t * x, lv_coord_t * y);

/**********************
 *  STATIC VARIABLES
 **********************/
lv_indev_t * indev_touchpad;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
static lv_coord_t tp_x = 0;
static lv_coord_t tp_y = 0;
static bool tp_pressed = false;

/* 刷新当前触摸状态：只在这里读 CST816T */
static void touch_update(void)
{
    uint16_t tx, ty;
    uint8_t gesture;

    // 只是顺便清标志位，不拿它当“门”
    if (touch_int_flag) {
        touch_int_flag = 0;
    }

    /* 读一次当前状态：有手指 -> pressed；没手指 -> released */
    if (CST816_GetAction(&tx, &ty, &gesture)) {
        tp_pressed = true;

        // 这里可以顺便做坐标映射/偏移，你先简单直接赋值：
        tp_x = (lv_coord_t)tx;
        tp_y = (lv_coord_t)ty;
    } else {
        tp_pressed = false;
        // 坐标可以保持上一次的值，让 LVGL 看到“松开时在最后那个点”
    }
}

void lv_port_indev_init(void)
{
    /**
     * Here you will find example implementation of input devices supported by LittelvGL:
     *  - Touchpad
     *  - Mouse (with cursor support)
     *  - Keypad (supports GUI usage only with key)
     *  - Encoder (supports GUI usage only with: left, right, push)
     *  - Button (external buttons to press points on the screen)
     *
     *  The `..._read()` function are only examples.
     *  You should shape them according to your hardware
     */

    static lv_indev_drv_t indev_drv;

    /*------------------
     * Touchpad
     * -----------------*/

    /*Initialize your touchpad if you have*/
    touchpad_init();

    /*Register a touchpad input device*/
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchpad_read;
    indev_touchpad = lv_indev_drv_register(&indev_drv);

}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/*------------------
 * Touchpad
 * -----------------*/

/*Initialize your touchpad*/
static void touchpad_init(void)
{
    CST816T_Init();/*Your code comes here*/
}

/*Will be called by the library to read the touchpad*/
static void touchpad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
    static lv_coord_t last_x = 0;
    static lv_coord_t last_y = 0;

    /*Save the pressed coordinates and the state*/
    if(touchpad_is_pressed()) {
        touchpad_get_xy(&last_x, &last_y);
        data->state = LV_INDEV_STATE_PR;
    }
    else {
        data->state = LV_INDEV_STATE_REL;
    }

    /*Set the last pressed coordinates*/
    data->point.x = last_x;
    data->point.y = last_y;
}

/*Return true if the touchpad is pressed*/
static bool touchpad_is_pressed(void)
{
    /* 每次 LVGL 读输入时先刷新一次触摸状态 */
    touch_update();

    return tp_pressed;
}

/*Get the x and y coordinates if the touchpad is pressed*/
static void touchpad_get_xy(lv_coord_t * x, lv_coord_t * y)
{
    *x = tp_x;
    *y = tp_y;
}

#else /*Enable this file at the top*/

/*This dummy typedef exists purely to silence -Wpedantic.*/
typedef int keep_pedantic_happy;
#endif
