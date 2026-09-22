#include"camera.h"
#include "esp_camera.h"
#include "esp_err.h"
#include "esp_log.h"

/* 引脚声明 */
#define CAM_PIN_PWDN GPIO_NUM_NC
#define CAM_PIN_RESET GPIO_NUM_NC
#define CAM_PIN_XCLK GPIO_NUM_NC
#define CAM_PIN_SIOD GPIO_NUM_39
#define CAM_PIN_SIOC GPIO_NUM_38
#define CAM_PIN_D7 GPIO_NUM_18
#define CAM_PIN_D6 GPIO_NUM_17
#define CAM_PIN_D5 GPIO_NUM_16
#define CAM_PIN_D4 GPIO_NUM_15
#define CAM_PIN_D3 GPIO_NUM_7
#define CAM_PIN_D2 GPIO_NUM_6
#define CAM_PIN_D1 GPIO_NUM_5
#define CAM_PIN_D0 GPIO_NUM_4
#define CAM_PIN_VSYNC GPIO_NUM_47
#define CAM_PIN_HREF GPIO_NUM_48
#define CAM_PIN_PCLK GPIO_NUM_45
#define CAM_PWDN(x) do{ x ? \
 (xl9555_pin_write(OV_PWDN_IO, 1)): \
 (xl9555_pin_write(OV_PWDN_IO, 0)); \
 }while(0)
#define CAM_RST(x) do{ x ? \
 (xl9555_pin_write(OV_RESET_IO, 1)): \
 (xl9555_pin_write(OV_RESET_IO, 0)); \
 }while(0)

static camera_config_t camera_config = {
    .pin_pwdn  = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sccb_sda = CAM_PIN_SIOD,
    .pin_sccb_scl = CAM_PIN_SIOC,

    .pin_d7 = CAM_PIN_D7,
    .pin_d6 = CAM_PIN_D6,
    .pin_d5 = CAM_PIN_D5,
    .pin_d4 = CAM_PIN_D4,
    .pin_d3 = CAM_PIN_D3,
    .pin_d2 = CAM_PIN_D2,
    .pin_d1 = CAM_PIN_D1,
    .pin_d0 = CAM_PIN_D0,
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_pclk = CAM_PIN_PCLK,

    .xclk_freq_hz = 24000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_RGB565,//YUV422,GRAYSCALE,RGB565,JPEG
    .frame_size = FRAMESIZE_QVGA,//QQVGA-UXGA, For ESP32, do not use sizes above QVGA when not JPEG. The performance of the ESP32-S series has improved a lot, but JPEG mode always gives better frame rates.

    .jpeg_quality = 12, //0-63, for OV series camera sensors, lower number means higher quality
    .fb_count = 2, //When jpeg mode is used, if fb_count more than one, the driver will work in continuous mode.
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,//CAMERA_GRAB_LATEST. Sets when buffers should be filled
    .fb_location=CAMERA_FB_IN_PSRAM
};
esp_err_t camera_init(void){
    esp_err_t ret;

    if (CAM_PIN_PWDN == GPIO_NUM_NC)
    {
       CAM_PWDN(0);
    }
    if (CAM_PIN_RESET == GPIO_NUM_NC)
    {
       CAM_RST(0);
       vTaskDelay(20);
       CAM_RST(1);
       vTaskDelay(20);
    }

    //initialize the camera
    ret = esp_camera_init(&camera_config);
    if (ret != ESP_OK)
    {
        ESP_LOGE("CAMERA", "init failed: %s", esp_err_to_name(ret));
    }
    return ret;
}

camera_fb_t * fb=NULL;
void camera_show(void){
    //acquire a frame
    fb = esp_camera_fb_get();
    //replace this with your own function
    lcd_show_picture(fb->buf);
    //return the frame buffer back to the driver for reuse
    esp_camera_fb_return(fb);
    fb=NULL;
}

/**
 * @brief       取一帧摄像头画面，只把前 rows 行显示到屏幕顶部（底部留给状态栏）
 * @param       rows：要显示的行数，须小于屏幕高度
 * @retval      ESP_OK  ：这一帧已显示到屏幕
 *              ESP_FAIL：取帧失败，调用方可重试（不允许当成成功静默跳过）
 */
esp_err_t camera_show_rows(uint16_t rows){
    camera_fb_t *frame = esp_camera_fb_get();

    if (frame == NULL)
    {
        return ESP_FAIL;
    }

    lcd_show_picture_rows(frame->buf, rows);
    esp_camera_fb_return(frame);
    return ESP_OK;
}
