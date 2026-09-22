#ifndef __CAMERA_H
#define __CAMERA_H
#include <stdio.h>
#include"freertos/FreeRTOS.h"
#include"freertos/task.h"
#include"driver/gpio.h"
#include"lcd.h"
#include"esp_camera.h"
#include"esp_err.h"

esp_err_t camera_init(void);
void camera_show(void);
esp_err_t camera_show_rows(uint16_t rows);




#endif
