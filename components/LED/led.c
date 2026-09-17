#include"led.h"
void led_init(void)
{
    esp_err_t err;
    gpio_config_t gpio_configstruct={
        .intr_type=GPIO_INTR_DISABLE,
        .mode=GPIO_MODE_INPUT_OUTPUT,
        .pin_bit_mask=1ull<<GPIO_NUM_1,
        .pull_down_en=GPIO_PULLDOWN_DISABLE,
        .pull_up_en=GPIO_PULLUP_ENABLE,     
    };
    err=gpio_config(&gpio_configstruct);
    if(err!=ESP_OK)
    {
      printf("gpio init fail\r\n");
    }
}

void gpio_toggle(gpio_num_t gpio_num)
{
    if(gpio_get_level(gpio_num)==1)
    {
        gpio_set_level(gpio_num,0);
    }
    else
    {
        gpio_set_level(gpio_num,1);
    }
}