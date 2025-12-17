#include "ui.h"
#include "lvgl.h"
#include <stdio.h>

/* ==================== 1. 参数配置 ==================== */
#define SCREEN_W 240
#define SCREEN_H 280

/* 下拉栏配置 (240x280) */
#define STATUS_PANEL_H  280 
#define STATUS_HIDDEN_Y (-280) 
#define STATUS_THRESHOLD (STATUS_HIDDEN_Y / 2) 

/* 左侧栏配置 (240x280) */
#define SIDE_PANEL_W    240 
#define SIDE_HIDDEN_X   (-240) 
#define SIDE_THRESHOLD  (SIDE_HIDDEN_X / 2)   

/* 触发灵敏度 */
#define DRAG_LIMIT_Y    60   
#define DRAG_LIMIT_X    40   

/* ==================== 2. 全局变量 ==================== */
static bool is_dragging = false;
static int  drag_axis = 0;   
static lv_point_t start_point = {0, 0};
static lv_coord_t panel_start_pos = 0; 

/* 状态标记 */
bool is_top_open = false;
bool is_left_open = false;

/* ==================== 3. 核心函数 ==================== */

static void anim_snap_to(lv_obj_t * obj, int32_t val, bool is_y)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_time(&a, 250); 
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);

    if (is_y) {
        lv_anim_set_values(&a, lv_obj_get_y(obj), val);
        lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_y);
    } else {
        lv_anim_set_values(&a, lv_obj_get_x(obj), val);
        lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_x);
    }
    lv_anim_start(&a);
}

void Global_Gesture_Handler(lv_event_t * e)
{
    /* 1. 获取当前点击的目标控件 */
    lv_obj_t * target = lv_event_get_target(e);

    /* ================= 【关键修改：白名单过滤】 ================= */
    /* 如果点击的是 Slider, Switch, Button 等交互控件，直接退出，不拖拽面板 */
    if (lv_obj_check_type(target, &lv_slider_class) || 
        lv_obj_check_type(target, &lv_switch_class) ||
        lv_obj_check_type(target, &lv_btn_class)) 
    {
        return; 
    }
    /* ========================================================== */

    lv_event_code_t code = lv_event_get_code(e);
    lv_indev_t * indev = lv_indev_get_act();
    if (indev == NULL) return;

    lv_point_t point;
    lv_indev_get_point(indev, &point);

    /* --------------- 按下 (PRESSING) --------------- */
    if (code == LV_EVENT_PRESSING) 
    {
        if (!is_dragging) 
        {
            /* 1. 如果下拉栏开着 -> 准备推回去 */
            if (is_top_open) {
                if (point.y < STATUS_PANEL_H) {
                    is_dragging = true; drag_axis = 1; 
                    start_point = point; panel_start_pos = 0;
                }
            }
            /* 2. 如果侧边栏开着 -> 准备推回去 */
            else if (is_left_open) {
                if (point.x < SIDE_PANEL_W) {
                    is_dragging = true; drag_axis = 2; 
                    start_point = point; panel_start_pos = 0;
                }
            }
            /* 3. 都是关着的 -> 检测边缘拉出 */
            else {
                if (point.y < DRAG_LIMIT_Y) {
                    is_dragging = true; drag_axis = 1; 
                    start_point = point; panel_start_pos = STATUS_HIDDEN_Y;
                }
                else if (point.x < DRAG_LIMIT_X) {
                    is_dragging = true; drag_axis = 2; 
                    start_point = point; panel_start_pos = SIDE_HIDDEN_X;
                }
            }
        }

        /* 拖拽中 */
        if (is_dragging) 
        {
            if (drag_axis == 1) { // 下拉栏
                lv_coord_t diff = point.y - start_point.y;
                lv_coord_t new_y = panel_start_pos + diff;
                
                if (new_y > 0) new_y = 0;
                if (new_y < STATUS_HIDDEN_Y) new_y = STATUS_HIDDEN_Y;
                
                lv_obj_set_y(ui_PanelStatus, new_y);
            }
            else if (drag_axis == 2) { // 左侧栏
                lv_coord_t diff = point.x - start_point.x;
                lv_coord_t new_x = panel_start_pos + diff;

                if (new_x > 0) new_x = 0;
                if (new_x < SIDE_HIDDEN_X) new_x = SIDE_HIDDEN_X;

                lv_obj_set_x(ui_PanelFunc, new_x);
            }
        }
    }
    /* --------------- 松手 (RELEASED) --------------- */
    else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) 
    {
        if (is_dragging) {
            is_dragging = false;

            // --- 结算下拉栏 ---
            if (drag_axis == 1) {
                lv_coord_t cur_y = lv_obj_get_y(ui_PanelStatus);
                if (cur_y > STATUS_THRESHOLD) { 
                    anim_snap_to(ui_PanelStatus, 0, true);
                    is_top_open = true;
                } else {
                    anim_snap_to(ui_PanelStatus, STATUS_HIDDEN_Y, true);
                    is_top_open = false;
                }
            }
            // --- 结算侧边栏 ---
            else if (drag_axis == 2) {
                lv_coord_t cur_x = lv_obj_get_x(ui_PanelFunc);
                if (cur_x > SIDE_THRESHOLD) {
                    anim_snap_to(ui_PanelFunc, 0, false);
                    is_left_open = true;
                } else {
                    anim_snap_to(ui_PanelFunc, SIDE_HIDDEN_X, false);
                    is_left_open = false;
                }
            }
            drag_axis = 0;
        }
    }
}

void Setup_Gesture_Logic(void)
{
    // 1. 监听屏幕
    if (ui_Screen1 != NULL) {
        lv_obj_add_event_cb(ui_Screen1, Global_Gesture_Handler, LV_EVENT_ALL, NULL);
    }

    // 2. 监听 Panel (必须配合 SLS 的 Clickable 属性!)
    if (ui_PanelStatus != NULL) {
        lv_obj_add_event_cb(ui_PanelStatus, Global_Gesture_Handler, LV_EVENT_ALL, NULL);
    }

    if (ui_PanelFunc != NULL) {
        lv_obj_add_event_cb(ui_PanelFunc, Global_Gesture_Handler, LV_EVENT_ALL, NULL);
    }
}
