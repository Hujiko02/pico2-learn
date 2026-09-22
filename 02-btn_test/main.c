#include <stdio.h>
#include "pico/stdlib.h"

#define LED_PIN 25
#define BTN_PIN 15
#define DEBOUNCE_MS  200

static volatile int g_pressd_flag = 0;

void btn_irq(uint gpio, uint32_t events)  //中断函数 BTN_PIN下降沿触发，标志位置为1
{
    static uint32_t last_ms = 0;    //软件消抖
    uint32_t now_ms = to_ms_since_boot(get_absolute_time());
    if (last_ms != 0 && (now_ms - last_ms) < DEBOUNCE_MS) return;
    last_ms = now_ms;
    
    g_pressd_flag = 1;
}

int main()
{
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    gpio_init(BTN_PIN);
    gpio_set_dir(BTN_PIN, GPIO_IN);
    gpio_pull_up(BTN_PIN);
    
    gpio_set_irq_enabled_with_callback(BTN_PIN, GPIO_IRQ_EDGE_FALL, true, &btn_irq);
    
    while (true) 
    {
        if(g_pressd_flag)
        {
            g_pressd_flag = 0;
            gpio_put(LED_PIN, !gpio_get(LED_PIN));
        }
    }
}

