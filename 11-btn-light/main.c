#include "pico/stdlib.h"
#include "led.h"
#include "key.h"

int main()
{
    led_init();
    key_init();

    uint8_t keynum = 0;
    while (true)
    {
        keynum = key_getnum();
        if(keynum == 1)
        {
            led1_toggle();
        }

        if(keynum == 2)
        {
            led2_toggle();
        }
    }
}
