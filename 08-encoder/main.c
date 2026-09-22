#include <stdio.h>
#include "pico/stdlib.h"

#define ENC_A 16
#define ENC_B 17

static volatile int g_count = 0;

void encoder_irq(uint gpio, uint32_t events)
{
    // 只在 A 相下降沿触发
    if (gpio == ENC_A && (events & GPIO_IRQ_EDGE_FALL)) {
        if (gpio_get(ENC_B)) {
            g_count++;      // 正转，具体方向可能相反，可调换
        } else {
            g_count--;      // 反转
        }
    }
}
int main()
{
    stdio_init_all();

    gpio_init(ENC_A);
    gpio_set_dir(ENC_A, GPIO_IN);
    gpio_pull_up(ENC_A);

    gpio_init(ENC_B);
    gpio_set_dir(ENC_B, GPIO_IN);
    gpio_pull_up(ENC_B);

    gpio_set_irq_enabled_with_callback(ENC_A, GPIO_IRQ_EDGE_FALL, true, &encoder_irq);

    int last_count = 0;
    while (true) {
        if (g_count != last_count) {
            last_count = g_count;
            printf("count = %d\n", g_count);
        }
        sleep_ms(10);   // 降低 CPU 占用
    }
}