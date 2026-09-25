#include "pico/stdlib.h"
#include "led.h"

int main()
{
    led_init();
    led1_on();
    led2_off();
    while (true)
    {
        led1_toggle();
        led2_toggle();
        sleep_ms(1000);
    }
}
