#include "ui.h"
#include "lvgl.h"
#include <stdio.h>

/* ==================== 1. 参数配置 ==================== */
#define SCREEN_W 240
#define SCREEN_H 280

/* 上拉栏 (Top) 配置 */
#define STATUS_HIDDEN_Y (-280)
#define STATUS_THRESHOLD (STATUS_HIDDEN_Y / 2)

/* 左侧栏 (Left) 配置 */
#define SIDE_HIDDEN_X   (-240)
#define SIDE_THRESHOLD  (SIDE_HIDDEN_X / 2)

/* 右侧栏 (Right) 配置 - 新增 */
#define RIGHT_HIDDEN_X  240
#define RIGHT_THRESHOLD (RIGHT_HIDDEN_X / 2)

/* 底部栏 (Bottom) 配置 - 新增 */
#define BOTTOM_HIDDEN_Y 280
#define BOTTOM_THRESHOLD (BOTTOM_HIDDEN_Y / 2)

/* 触发灵敏度 */
#define DRAG_LIMIT_Y    60
#define DRAG_LIMIT_X    40

/* ==================== 2. 全局变量 ==================== */
static bool is_dragging = false;

/* * drag_axis 定义: 
 * 0=无, 1=Top(Y轴), 2=Left(X轴), 3=Right(X轴), 4=Bottom(Y轴) 
 */
static int  drag_axis = 0;   
static lv_point_t start_point = {0, 0};
static lv_coord_t panel_start_pos = 0; 

/* 状态标记 */
bool is_top_open = false;
bool is_left_open = false;
bool is_right_open = false;   // 新增
bool is_bottom_open = false;  // 新增

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
    /* 1. 白名单过滤：防止拖拽Slider/Button时误触发 */
    lv_obj_t * target = lv_event_get_target(e);
    if (lv_obj_check_type(target, &lv_slider_class) || 
        lv_obj_check_type(target, &lv_switch_class) ||
        lv_obj_check_type(target, &lv_btn_class)) 
    {
        return; 
    }

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
            /* A. 处理【已打开】状态 -> 准备推回去 */
            if (is_top_open) {
                // 点击任意位置都可以开始推回去，这里简单判断
                is_dragging = true; drag_axis = 1; 
                start_point = point; panel_start_pos = 0;
            }
            else if (is_left_open) {
                is_dragging = true; drag_axis = 2; 
                start_point = point; panel_start_pos = 0;
            }
            else if (is_right_open) {
                is_dragging = true; drag_axis = 3; 
                start_point = point; panel_start_pos = 0;
            }
            else if (is_bottom_open) {
                is_dragging = true; drag_axis = 4; 
                start_point = point; panel_start_pos = 0;
            }
            
            /* B. 处理【关闭】状态 -> 检测边缘拉出 */
            else {
                // 1. 上边缘
                if (point.y < DRAG_LIMIT_Y) {
                    is_dragging = true; drag_axis = 1; 
                    start_point = point; panel_start_pos = STATUS_HIDDEN_Y;
                }
                // 2. 下边缘 (屏幕高度 - 阈值)
                else if (point.y > (SCREEN_H - DRAG_LIMIT_Y)) {
                    is_dragging = true; drag_axis = 4; 
                    start_point = point; panel_start_pos = BOTTOM_HIDDEN_Y;
                }
                // 3. 左边缘
                else if (point.x < DRAG_LIMIT_X) {
                    is_dragging = true; drag_axis = 2; 
                    start_point = point; panel_start_pos = SIDE_HIDDEN_X;
                }
                // 4. 右边缘 (屏幕宽度 - 阈值)
                else if (point.x > (SCREEN_W - DRAG_LIMIT_X)) {
                    is_dragging = true; drag_axis = 3; 
                    start_point = point; panel_start_pos = RIGHT_HIDDEN_X;
                }
            }
        }

        /* 拖拽中逻辑 */
        if (is_dragging) 
        {
            /* Case 1: 上拉栏 (负坐标 -> 0) */
            if (drag_axis == 1) { 
                lv_coord_t diff = point.y - start_point.y;
                lv_coord_t new_y = panel_start_pos + diff;
                if (new_y > 0) new_y = 0;
                if (new_y < STATUS_HIDDEN_Y) new_y = STATUS_HIDDEN_Y;
                lv_obj_set_y(ui_PanelStatus, new_y);
            }
            /* Case 2: 左侧栏 (负坐标 -> 0) */
            else if (drag_axis == 2) { 
                lv_coord_t diff = point.x - start_point.x;
                lv_coord_t new_x = panel_start_pos + diff;
                if (new_x > 0) new_x = 0;
                if (new_x < SIDE_HIDDEN_X) new_x = SIDE_HIDDEN_X;
                lv_obj_set_x(ui_PanelFunc, new_x);
            }
            /* Case 3: 右侧栏 (正坐标240 -> 0) */
            else if (drag_axis == 3) { 
                lv_coord_t diff = point.x - start_point.x; // 向左滑 diff 是负数
                lv_coord_t new_x = panel_start_pos + diff;
                
                if (new_x < 0) new_x = 0;           // 不能超过左边
                if (new_x > RIGHT_HIDDEN_X) new_x = RIGHT_HIDDEN_X; // 不能超过右边隐藏位
                
                lv_obj_set_x(ui_PanelRight, new_x);
            }
            /* Case 4: 底部栏 (正坐标280 -> 0) */
            else if (drag_axis == 4) { 
                lv_coord_t diff = point.y - start_point.y; // 向上滑 diff 是负数
                lv_coord_t new_y = panel_start_pos + diff;

                if (new_y < 0) new_y = 0;           // 不能超过顶边
                if (new_y > BOTTOM_HIDDEN_Y) new_y = BOTTOM_HIDDEN_Y; // 不能超过底边隐藏位

                lv_obj_set_y(ui_PanelBottom, new_y);
            }
        }
    }
    /* --------------- 松手 (RELEASED) --------------- */
    else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) 
    {
        if (is_dragging) {
            is_dragging = false;

            /* --- 1. 结算 Top --- */
            if (drag_axis == 1) {
                lv_coord_t cur_y = lv_obj_get_y(ui_PanelStatus);
                // 阈值判断：如果拉下来超过了一半 (阈值是负数，例如 -140，大于它意味着更接近0)
                if (cur_y > STATUS_THRESHOLD) { 
                    anim_snap_to(ui_PanelStatus, 0, true);
                    is_top_open = true;
                } else {
                    anim_snap_to(ui_PanelStatus, STATUS_HIDDEN_Y, true);
                    is_top_open = false;
                }
            }
            /* --- 2. 结算 Left --- */
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
            /* --- 3. 结算 Right (坐标范围 0 ~ 240) --- */
            else if (drag_axis == 3) {
                lv_coord_t cur_x = lv_obj_get_x(ui_PanelRight);
                // 阈值是 120。如果小于120（说明拉得很靠左了），就全开
                if (cur_x < RIGHT_THRESHOLD) {
                    anim_snap_to(ui_PanelRight, 0, false);
                    is_right_open = true;
                } else {
                    anim_snap_to(ui_PanelRight, RIGHT_HIDDEN_X, false);
                    is_right_open = false;
                }
            }
            /* --- 4. 结算 Bottom (坐标范围 0 ~ 280) --- */
            else if (drag_axis == 4) {
                lv_coord_t cur_y = lv_obj_get_y(ui_PanelBottom);
                // 阈值是 140。如果小于140（说明拉得很靠上了），就全开
                if (cur_y < BOTTOM_THRESHOLD) {
                    anim_snap_to(ui_PanelBottom, 0, true);
                    is_bottom_open = true;
                } else {
                    anim_snap_to(ui_PanelBottom, BOTTOM_HIDDEN_Y, true);
                    is_bottom_open = false;
                }
            }
            
            drag_axis = 0;
        }
    }
}

void Setup_Gesture_Logic(void)
{
    // 1. 监听屏幕背景 (处理关闭状态下的拉出)
    if (ui_Screen1 != NULL) {
        lv_obj_add_event_cb(ui_Screen1, Global_Gesture_Handler, LV_EVENT_ALL, NULL);
    }

    // 2. 监听各 Panel (处理打开状态下的推回)
    if (ui_PanelStatus != NULL) lv_obj_add_event_cb(ui_PanelStatus, Global_Gesture_Handler, LV_EVENT_ALL, NULL);
    if (ui_PanelFunc   != NULL) lv_obj_add_event_cb(ui_PanelFunc,   Global_Gesture_Handler, LV_EVENT_ALL, NULL);
    
    // 新增监听
    if (ui_PanelRight  != NULL) lv_obj_add_event_cb(ui_PanelRight,  Global_Gesture_Handler, LV_EVENT_ALL, NULL);
    if (ui_PanelBottom != NULL) lv_obj_add_event_cb(ui_PanelBottom, Global_Gesture_Handler, LV_EVENT_ALL, NULL);
}
