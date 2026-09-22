/*
 * Day 7 MVP：摄像头实时画面 + 底部状态栏
 * 对应 PRD：F1（实时画面采集）、F7（状态指示，Day 7 只落地「待机 / 错误」两态）
 *
 * 屏幕分区（横屏 320 x 240）：
 *   y =   0 ~ 215   视频区：摄像头画面，每帧刷新
 *   y = 216 ~ 239   状态栏：只在状态变化时重画一次，不会被视频覆盖
 *
 * 说明：板载字库只有 ASCII（components/LCD/lcdfont.h 里是 asc2_xxxx），
 *       所以状态文字暂用英文，PRD 要求的「请看镜头」等中文文案后续再接中文字库。
 */
#include <stdio.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "led.h"
#include "lcd.h"
#include "camera.h"

static const char *TAG = "APP";

i2c_obj_t i2c0_master;

#define VIDEO_ROWS          216                     /* 216 行 x 320 x 2 = 138240 B = 9 x LCD_BUF_SIZE，正好整块写 */
#define BAR_Y               VIDEO_ROWS
#define PREVIEW_TASK_STACK  4096
#define PREVIEW_TASK_PRIO   5

/* PRD 2.1 是 4 个正常态 + 1 个错误态；Day 7 先落地不依赖人脸检测的两态，
   录入中 / 识别成功 / 识别失败 等人脸检测接上后再加 */
typedef enum {
    ST_STANDBY = 0,
    ST_ERROR,
} app_state_t;

static volatile app_state_t s_state = ST_STANDBY;

static const char *state_text(app_state_t st)
{
    switch (st)
    {
        case ST_STANDBY: return "STANDBY - LOOK AT CAMERA";
        case ST_ERROR:   return "ERROR";
        default:         return "UNKNOWN";
    }
}

/* 状态栏：只在状态变化时画一次。视频只刷上方 216 行，所以不会被冲掉 */
static void ui_draw_bar(void)
{
    lcd_fill(0, BAR_Y, lcd_self.width - 1, lcd_self.height - 1, BLACK);
    lcd_show_string(8, BAR_Y + 4, lcd_self.width - 16, 16, 16, (char *)state_text(s_state), WHITE);
}

/* 错误态整屏静态页：没画面可显示，但按 PRD 2.4 第一条总原则，不允许静默失败 */
static void ui_show_error_page(void)
{
    lcd_clear(WHITE);
    lcd_draw_rectangle(4, 4, 315, 235, BLACK);
    lcd_show_string(16, 48, 288, 32, 32, "CAMERA ERROR", BLACK);
    lcd_show_string(16, 120, 288, 16, 16, "POWER CYCLE TO RETRY", BLACK);
}

/* 预览任务：取帧 -> 刷屏 -> 每秒打一行状态和实测帧率 */
static void preview_task(void *arg)
{
    uint32_t frames = 0;
    int64_t  t_last = esp_timer_get_time();

    while (1)
    {
        int64_t now;

        if (s_state != ST_STANDBY)
        {
            vTaskDelay(pdMS_TO_TICKS(200));
            continue;
        }

        if (camera_show_rows(VIDEO_ROWS) != ESP_OK)
        {
            ESP_LOGW(TAG, "frame grab failed, retry");      /* 取帧失败不静默 */
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }
        frames++;

        now = esp_timer_get_time();
        if (now - t_last >= 1000000)
        {
            ESP_LOGI(TAG, "state=%s fps=%" PRIu32, state_text(s_state),
                     (uint32_t)((uint64_t)frames * 1000000ULL / (uint64_t)(now - t_last)));
            frames = 0;
            t_last = now;
        }
    }
}

void app_main(void)
{
    esp_err_t ret;

    ESP_LOGI(TAG, "boot");

    led_init();
    i2c0_master = iic_init(I2C_NUM_0);
    xl9555_init(i2c0_master);
    spi2_init();

    ret = lcd_init();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "lcd init failed: %s", esp_err_to_name(ret));
        while (1)
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    ret = camera_init();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "camera init failed: %s", esp_err_to_name(ret));
        s_state = ST_ERROR;
        ui_show_error_page();
        ESP_LOGI(TAG, "state=%s", state_text(s_state));
    }
    else
    {
        s_state = ST_STANDBY;
        lcd_clear(BLACK);                                   /* 视频区先清黑，避免开局花屏 */
        ui_draw_bar();
        ESP_LOGI(TAG, "camera ready, state=%s", state_text(s_state));
        xTaskCreatePinnedToCore(preview_task, "preview", PREVIEW_TASK_STACK, NULL,
                                PREVIEW_TASK_PRIO, NULL, 1);
    }

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
