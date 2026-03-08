#include "gui.h"
#include "lvgl.h"
#include "w25q64.h"

lv_obj_t *switch_obj = NULL;
lv_obj_t *label_time = NULL;
lv_obj_t *label_spo2 = NULL;
lv_obj_t *label_hr   = NULL;
ClockTime_t g_clock_time = {0};

void gui_init(void)
{
		lv_obj_t* switch_obj = lv_switch_create(lv_scr_act());
		lv_obj_set_size(switch_obj, 120, 50);
		lv_obj_align(switch_obj, LV_ALIGN_BOTTOM_MID, 0, -10);

    // 创建一个简单界面示例 
    label_time = lv_label_create(lv_scr_act());
    lv_obj_align(label_time, LV_ALIGN_TOP_MID, 0, 10);

    label_spo2 = lv_label_create(lv_scr_act());
    lv_obj_align(label_spo2, LV_ALIGN_LEFT_MID, 10, 0);

    label_hr = lv_label_create(lv_scr_act());
    lv_obj_align(label_hr, LV_ALIGN_RIGHT_MID, -10, 0);

    lv_label_set_text(label_time, "00:00:00");
    lv_label_set_text(label_spo2, "SpO2: --%");
    lv_label_set_text(label_hr,   "HR: --");

}

// 定义一个图片描述符
lv_img_dsc_t my_external_img = {
  .header.always_zero = 0,
  .header.w = 240,
  .header.h = 280,
  .header.cf = LV_IMG_CF_TRUE_COLOR, // 对应 RGB565
  .data_size = 134400,
  .data = (const uint8_t *)0x000000, // 这里存资源在 W25Q64 里的起始地址
};

// 定义 Info 回调：告诉 LVGL 图片的尺寸和格式
static lv_res_t my_decoder_info(lv_img_decoder_t * decoder, const void * src, lv_img_header_t * header)
{
    if(src == &my_external_img) {
        header->cf = LV_IMG_CF_TRUE_COLOR; // 对应你的 RGB565 [cite: 35]
        header->w = 240;
        header->h = 280;
        return LV_RES_OK;
    }
    return LV_RES_INV;
}

// 定义 Open 回调：校验资源
static lv_res_t my_decoder_open(lv_img_decoder_t * decoder, lv_img_decoder_dsc_t * dsc)
{
    if(dsc->src == &my_external_img) {
        return LV_RES_OK;
    }
    return LV_RES_INV;
}

// 核心算法：分块加载（Read Line）
static lv_res_t my_decoder_read_line(lv_img_decoder_t * decoder, lv_img_decoder_dsc_t * dsc,
                                     lv_coord_t x, lv_coord_t y, lv_coord_t len, uint8_t * buf)
{
    // 将 void* 强转为图片描述符以获取在 .data 里存的 W25Q64 起始地址
    const lv_img_dsc_t * img_dsc = dsc->src; 
    uint32_t base_addr = (uint32_t)img_dsc->data; 
    
    // 计算地址偏移：y * 宽度 * 2字节 (RGB565每像素2字节)
    uint32_t offset = base_addr + (y * 240 * 2);

    // 从外部 Flash 读取这一行的数据 [
    W25Q_ReadData(offset, buf, len * 2); 

    return LV_RES_OK;
}

// 初始化并注册解码器
void my_decoder_init(void)
{
    lv_img_decoder_t * decoder = lv_img_decoder_create();
    lv_img_decoder_set_info_cb(decoder, my_decoder_info); 
    lv_img_decoder_set_open_cb(decoder, my_decoder_open); 
    lv_img_decoder_set_read_line_cb(decoder, my_decoder_read_line); 
}
