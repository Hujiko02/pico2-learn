/*
 * 02-btn —— 按键中断（对应 STM32 标准库的 EXTI）
 *
 * 接线：
 *      GP14 ──┤ 按键 ├── GND        （用芯片内部上拉，不用外接电阻）
 *      板载 LED 在 GP25，不需要接线
 *
 * 现象：
 *      按一下按键 → LED 翻转一次 → 串口打印一行「第 N 次按下」
 *
 * 【和 STM32 标准库 EXTI 对比】
 *   SPL 要 5 步 + 3 个结构体 + 手写中断向量函数：
 *       1. RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);   开 AFIO 时钟
 *       2. GPIO_Init(GPIOA, &GPIO_InitStructure);                配成输入
 *       3. GPIO_EXTILineConfig(EXTI_PortSourceGPIOA, EXTI_PinSource0);  绑线
 *       4. EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
 *          EXTI_Init(&EXTI_InitStructure);                       配触发沿
 *       5. NVIC_Init(&NVIC_InitStructure);                        开 NVIC
 *       + void EXTI0_IRQHandler(void) { ... EXTI_ClearITPendingBit(...); }
 *
 *   Pico SDK 只要 1 行：
 *       gpio_set_irq_enabled_with_callback(14, GPIO_IRQ_EDGE_FALL, true, &cb);
 *   时钟、AFIO 绑线、NVIC、清中断标志 全被吞进这个函数里了。
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define LED_PIN     25          /* 板载 LED */
#define BTN_PIN     14          /* 按键，另一端接 GND */

#define DEBOUNCE_MS 200         /* 消抖窗口。机械按键抖动一般 < 10ms，200ms 还能连点不误触 */

/* ── 中断服务函数里【只做最少的事】────────────────────────────
 *    这里只记数、置标志，真正的活儿交给主循环。
 *
 *    千万别在中断里 printf：
 *      · printf 要几十微秒到几毫秒，会阻塞其他中断
 *      · USB printf 内部还要等端点，时间不确定
 *      · 官方 gpio/hello_gpio_irq 例子确实在回调里 printf —— 学习可以，
 *        但那是为了演示；STM32 上也没人往 EXTI0_IRQHandler 里塞 printf。
 */
static volatile uint32_t g_press_count  = 0;
static volatile int      g_pressed_flag = 0;

static void btn_irq(uint gpio, uint32_t events) {
    (void)gpio; (void)events;

    /* 消抖：软件方式，比 STM32 常见的外部 RC 电路省事 */
    static uint32_t last_ms = 0;
    uint32_t now_ms = to_ms_since_boot(get_absolute_time());
    if (last_ms != 0 && (now_ms - last_ms) < DEBOUNCE_MS) return;
    last_ms = now_ms;

    g_press_count++;
    g_pressed_flag = 1;         /* 通知主循环 */
}

int main(void) {
    stdio_init_all();

    /* 等 USB 枚举完再打印，否则开头几行会丢（最多等 5 秒）*/
    for (int i = 0; i < 50 && !stdio_usb_connected(); i++) sleep_ms(100);

    /* ── LED
     * 【对比 SPL】gpio_init() 一次做完「开外设时钟 + 复位引脚」，
     *             对应 SPL 的 RCC_APB2PeriphClockCmd + GPIO_DeInit */
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);

    /* ── 按键：内部上拉，按下接地 → 常态高电平，按下变低 → 检测下降沿
     * 【对比 SPL】SPL 的 GPIO_Mode_IPU（上拉输入）就是 gpio_pull_up() */
    gpio_init(BTN_PIN);
    gpio_set_dir(BTN_PIN, GPIO_IN);
    gpio_pull_up(BTN_PIN);

    /* ── 一行注册中断：时钟 + 绑线 + 触发沿 + NVIC + 回调函数 */
    gpio_set_irq_enabled_with_callback(BTN_PIN, GPIO_IRQ_EDGE_FALL, true, &btn_irq);

    printf("02-btn 起来了\n");
    printf("按键接在 GP%d（另一端接 GND）\n", BTN_PIN);
    printf("现在引脚电平 = %d，没按应该是 1（上拉）\n", gpio_get(BTN_PIN));

    int led_on = 0;
    while (true) {
        if (g_pressed_flag) {
            g_pressed_flag = 0;
            led_on = !led_on;
            gpio_put(LED_PIN, led_on);
            printf("第 %lu 次按下，LED %s\n",
                   (unsigned long)g_press_count, led_on ? "亮" : "灭");
        }
        sleep_ms(10);           /* 主循环摸鱼，不空转烧 CPU */
    }
    return 0;
}
