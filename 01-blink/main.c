/*
 * 01-blink —— 板载 LED 闪烁 + 串口打印计数
 *
 * 硬件：不需要任何外接元件（Pico 2 板载 LED 接在 GP25）
 * 现象：LED 每 0.5 秒翻转一次；串口 115200 每秒打印一行计数
 *
 * 【和 STM32 标准库对比】
 *   SPL 点灯要三步：
 *       RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);  // 1. 开外设时钟
 *       GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;          // 2. 配模式
 *       GPIO_Init(GPIOC, &GPIO_InitStruct);                    // 3. 初始化
 *       while(1){ GPIO_SetBits(GPIOC,GPIO_Pin_13); Delay(500); ... }
 *   Pico SDK 只要两步：gpio_init() 把时钟和复位一起做了，gpio_set_dir() 配方向。
 */

#include <stdio.h>
#include "pico/stdlib.h"

#define LED_PIN 25                   /* Pico 2 板载 LED = GP25 */

int main(void) {
    stdio_init_all();                /* 初始化 USB/串口输出 */
    gpio_init(LED_PIN);              /* 开时钟 + 复位引脚 */
    gpio_set_dir(LED_PIN, GPIO_OUT); /* 配成推挽输出 */

    printf("01-blink 起来了\n");

    int n = 0;
    while (true) {
        gpio_put(LED_PIN, 1);        /* 等价 GPIO_SetBits */
        sleep_ms(500);
        gpio_put(LED_PIN, 0);        /* 等价 GPIO_ResetBits */
        sleep_ms(500);
        printf("闪烁 %d 次\n", ++n);
    }
    return 0;
}
