/*
 * 02-btn —— 按键中断（对应 STM32 标准库的 EXTI）
 *
 * 接线：
 *      GP14 ──┤ 按键 ├── GND        （用芯片内部上拉，不用外接电阻）
 *      板载 LED 在 GP25，不需要接线
 *
 * 现象：
 *      按一下按键 → LED 翻转一次
 *
 *   Pico SDK 只要 1 行：
 *       gpio_set_irq_enabled_with_callback(14, GPIO_IRQ_EDGE_FALL, true, &cb);
 *   时钟、AFIO 绑线、NVIC、清中断标志 全在这个函数里。
 */

#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define LED_PIN 25          // 板载 LED 
#define BTN_PIN 16          // 按键，另一端接 GND
#define DEBOUNCE_MS 200     // 消抖窗口。机械按键抖动一般 < 10ms，200ms 还能连点不误触

static volatile int g_pressed_flag = 0;     

static void btn_irq(uint gpio, uint32_t events)   //中断函数 BTN_PIN下降沿触发，标志位置为1
{

    static uint32_t last_ms = 0;        // 消抖：软件方式
    uint32_t now_ms = to_ms_since_boot(get_absolute_time());
    if (last_ms != 0 && (now_ms - last_ms) < DEBOUNCE_MS) return;
    last_ms = now_ms;

    g_pressed_flag = 1;      // 通知主循环
}

int main(void) 
{
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);

    //按键：内部上拉，按下接地 → 常态高电平，按下变低 → 检测下降沿
    gpio_init(BTN_PIN);
    gpio_set_dir(BTN_PIN, GPIO_IN);
    gpio_pull_up(BTN_PIN);

    //行注册中断：时钟 + 绑线 + 触发沿 + NVIC + 回调函数 
    gpio_set_irq_enabled_with_callback(BTN_PIN, GPIO_IRQ_EDGE_FALL, true, &btn_irq);

    while (true) 
	{
        if (g_pressed_flag) 
        {
            g_pressed_flag = 0;
            gpio_put(LED_PIN, !gpio_get(LED_PIN));
        }
        sleep_ms(10);
    }
    return 0;
}
