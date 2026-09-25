#include "pico/stdlib.h"
#define KEY1_PIN 18
#define KEY2_PIN 16

void key_init(void)
{
    gpio_init(KEY1_PIN);
    gpio_set_dir(KEY1_PIN, GPIO_IN);
    gpio_pull_up(KEY1_PIN);
    
    gpio_init(KEY2_PIN);
    gpio_set_dir(KEY2_PIN, GPIO_IN);
    gpio_pull_up(KEY2_PIN);
}

uint8_t key_getnum()
{
    uint8_t keynum = 0;
    if(gpio_get(KEY1_PIN) == 0)
    {
        sleep_ms(10);
        while(gpio_get(KEY1_PIN) != 1);
        sleep_ms(10);
        keynum = 1;
    }

    if(gpio_get(KEY2_PIN) == 0)
    {
        sleep_ms(10);
        while(gpio_get(KEY2_PIN) != 1);
        sleep_ms(10);
        keynum = 2;
    }
    return keynum;
}