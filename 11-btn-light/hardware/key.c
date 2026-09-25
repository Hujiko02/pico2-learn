#include "pico/stdlib.h"
#define KEY1_PIN 10
#define KEY2_PIN 13

void key_init(void)
{
    gpio_init(KEY1_PIN);
    gpio_set_dir(KEY1_PIN, GPIO_IN);
    gpio_pull_up(KEY1_PIN);

    gpio_init(KEY2_PIN);
    gpio_set_dir(KEY2_PIN, GPIO_IN);
    gpio_pull_up(KEY2_PIN);
    
}