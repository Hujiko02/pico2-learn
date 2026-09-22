#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define COUNT 26

int main()
{
    int leds[COUNT] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,26,27,28};
    for(int i = 0; i < COUNT; i++)
    {
        gpio_init(leds[i]);
        gpio_set_dir(leds[i], GPIO_OUT);
    }

    int current = 0;

    while (true)
    {
        for(int i = 0; i < COUNT; i++)
        {
            gpio_put(leds[i], i == current ? 1 : 0);
        }
        sleep_ms(100);
        current = (current + 1) % COUNT;        
         
    } 
}