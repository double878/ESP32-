#include <stdio.h>
#include"freertos/FreeRTOS.h"
#include"freertos/task.h"
#include"driver/gpio.h"
#include"led.h"
#include"lcd.h"
#include"camera.h"
i2c_obj_t i2c0_master;
void app_main(void)
{
    led_init();
    i2c0_master = iic_init(I2C_NUM_0);
    xl9555_init(i2c0_master);
    spi2_init();
    lcd_init();
    camera_init();
    while(1)
    {
       
    }
}
    
