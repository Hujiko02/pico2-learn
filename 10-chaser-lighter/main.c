#include <stdio.h>
#include "pico/stdlib.h"


int main()
{
    gpio_init_mask(0xFF);
    gpio_set_dir_masked(0xFF, 0xFF);

    while (true) 
    {
        for(uint32_t i = 0x01; i != 0x100; i <<= 1)
        {
            gpio_put_masked(0xFF, i);
            sleep_ms(50);
        }

        for(uint32_t i = 0x80; i != 0x00; i >>= 1)
        {
            gpio_put_masked(0xFF, i);
            sleep_ms(50);
        }

    }
}
