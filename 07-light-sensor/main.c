/* ============================================================================
 *  07-light-sensor —— 光敏电阻 → 自动小夜灯（入门第 7 课）
 *
 *  ── 接线（6 根线）──────────────────────────────────────────────────────
 *      光敏模块是 4 个脚，看模块丝印：VCC / GND / DO / AO
 *
 *          VCC ──────── 3V3(OUT)   物理引脚 36
 *          GND ──────── GND        物理引脚 38
 *          AO  ──────── GP27       物理引脚 32   ← 模拟输出（我们主要用这个）
 *          DO  ──────── GP16       物理引脚 21   ← 数字输出（用来对比，可选）
 *
 *      外接 LED（5mm 那种，两脚，长脚是正极）：
 *          长脚(正极) ── GP15     物理引脚 20
 *          短脚(负极) ── 220Ω 或 330Ω 电阻 ── GND
 *
 *      ⚠️ 那个电阻【必须接】。LED 没有限流电阻会瞬间烧掉，还可能烧坏引脚。
 *         算一下：GPIO 输出 3.3V，白色/蓝色 LED 压降约 3.0V，想给 10mA，
 *         电阻 = (3.3 - 3.0) / 0.01 = 30Ω —— 但这样余量太小，电压一波动
 *         电流就爆了。用 220Ω 更安全（电流约 1.4mA，暗一点但绝对不烧）。
 *         红色 LED 压降只有 1.8V，(3.3-1.8)/220 ≈ 6.8mA，挺亮。
 *
 *  ── 现象 ──────────────────────────────────────────────────────────────
 *      串口打印，用手遮住光敏电阻：
 *
 *          AO=3812  DO=1  → 亮  [###############] LED 灭
 *          AO=1204  DO=0  → 暗  [#####          ] LED 亮
 *
 *      手一遮 LED 自动亮，移开自动灭。串口同时打印 DO 的状态做对比。
 *
 *  ══ ★ 本节最重要的知识点：AO 和 DO 有什么区别 ══
 *
 *      这个模块板上有一颗【LM393 比较器】和一个小电位器，所以：
 *
 *        AO（Analog Output）          DO（Digital Output）
 *        ──────────────────────       ──────────────────────────────
 *        0 ~ 3.3V 连续变化            只有 0 或 1 两种值
 *        接 ADC 引脚读                接普通 GPIO 读
 *        阈值由【你的程序】决定 ★      阈值由【模块上那个小电位器】决定
 *        能知道"多暗"                 只能知道"比阈值暗还是亮"
 *
 *      ★ 所以做正经项目都用 AO。DO 唯一的好处是不占 ADC 引脚、
 *        不用写代码 —— 但阈值要拿螺丝刀拧电位器调，很麻烦。
 *
 *  ══ 第二个知识点：迟滞（hysteresis）—— 防止临界点闪烁 ══
 *
 *      如果只用【一个】阈值：
 *          AO < 2000 → LED 亮      // 手停在临界点附近时，
 *                                  // 读数在 1999/2001 之间抖，
 *                                  // LED 就会疯狂闪，看着像坏了
 *
 *      用【两个】阈值（本项目就是这么做的）：
 *          AO < 1800 → 开灯          // 要"足够暗"才开
 *          AO > 2400 → 关灯          // 要"足够亮"才关
 *          1800 ~ 2400 之间 → 保持原样（不动作）
 *
 *      中间这段叫「死区」，两个阈值的差值叫「回差」。
 *      这不是偷懒，是工程上必须的 —— 温控器、电池充电器、
 *      房间灯的自动开关全都这么做。
 *
 *  ══ 必须自己实测的两个数字 ══
 *      不同模块的 AO 电压和光照【方向可能相反】：
 *        有的模块越亮 AO 越大，有的越亮 AO 越小（看板上分压怎么接的）。
 *      所以下面 TH_DARK / TH_BRIGHT 我填的是「越亮数值越大」的情况。
 *      你接好线先跑一次，看串口：
 *          遮住 → AO 变小还是变大？
 *          然后照着实测值改这两个宏，方向反了就把它们对调。
 *
 *  ══ 和 STM32 标准库的对比 ══
 *      和 05-adc-pot 一样，ADC 部分是 5 步 → 3 步。
 *      多出来的只是「读 GPIO」和「写 GPIO」，两边都差不多：
 *          SPL : GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_0)
 *          SDK : gpio_get(pin)
 *          SPL : GPIO_SetBits / GPIO_ResetBits
 *          SDK : gpio_put(pin, 0/1)
 * ========================================================================== */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"

#define LDR_AO_PIN   27     /* 光敏模块 AO 接 GP27 */
#define LDR_AO_CH     1     /* GP27 就是 ADC1 —— 这个数字硬件写死的 */

#define LDR_DO_PIN   16     /* 光敏模块 DO 接 GP16（只用来对比）*/
#define LED_PIN      15     /* 外接 LED 正极接 GP15 */

/* ── 两个阈值（迟滞）。★ 请根据实测值修改！
 *
 * 下面填的是「越亮 AO 越大」的情况：
 *     暗（遮住）  → AO 变小，比如 800
 *     亮（手拿开）→ AO 变大，比如 3500
 * 所以：
 *     AO 掉到 1800 以下 = 确实暗了 → 开灯
 *     AO 升到 2400 以上 = 确实亮了 → 关灯
 *
 * 如果你的模块方向相反（越亮 AO 越小），把这两个数改成：
 *     TH_DARK   2600     // 变亮后数值大了，超过它就说明"暗"
 *     TH_BRIGHT 2000
 * 然后把下面的判断逻辑也反过来。先跑一次看串口就明白了。 */
#define TH_DARK     1800
#define TH_BRIGHT   2400

int main() {
    stdio_init_all();

    /* ── LED */
    gpio_init(LED_PIN);
    gpio_put(LED_PIN, 0);               // 先熄灭再设成输出，避免上电闪一下
    gpio_set_dir(LED_PIN, GPIO_OUT);

    /* ── DO 引脚：模块的输出是推挽的，直接当输入读就行
     * 不用上拉 —— 模块自己会驱动成高或低 */
    gpio_init(LDR_DO_PIN);
    gpio_set_dir(LDR_DO_PIN, GPIO_IN);

    /* ── ADC（和 05-adc-pot 完全一样的三句）*/
    adc_init();
    adc_gpio_init(LDR_AO_PIN);
    adc_select_input(LDR_AO_CH);

    /* 等 USB 串口连上 */
    for (int i = 0; i < 150 && !stdio_usb_connected(); i++) sleep_ms(100);

    printf("\n");
    printf("==========================================\n");
    printf("  07-light-sensor   光敏 → 自动小夜灯\n");
    printf("==========================================\n");
    printf("接线：模块 VCC→3V3(36) GND→GND(38) AO→GP%d(32) DO→GP%d(21)\n",
           LDR_AO_PIN, LDR_DO_PIN);
    printf("      LED 长脚→GP%d(20)  短脚→220Ω→GND\n", LED_PIN);
    printf("遮挡光敏电阻试试，LED 应该自动亮起来\n");
    printf("★ 如果方向反了（挡光不亮、拿开才亮），按串口实测值改\n");
    printf("   main.c 里的 TH_DARK / TH_BRIGHT 两个宏\n");
    printf("------------------------------------------\n");

    bool led_on = false;
    int  last_ao = -1;              /* 上次打印的值，用来做变化检测 */
    int  last_do = -1;

    while (true) {
        /* 取 8 次平均，和 05 一样，让读数稳一点 */
        uint32_t sum = 0;
        for (int i = 0; i < 8; i++) sum += adc_read();
        int ao = (int)(sum / 8);

        int  do_val = gpio_get(LDR_DO_PIN);

        /* ── 迟滞判断：不是简单的 if (ao < 阈值) */
        if (!led_on && ao < TH_DARK) {
            /* 现在灯是灭的，只有"确实够暗"才开 */
            led_on = true;
            gpio_put(LED_PIN, 1);
        } else if (led_on && ao > TH_BRIGHT) {
            /* 现在灯是亮的，只有"确实够亮"才关 */
            led_on = false;
            gpio_put(LED_PIN, 0);
        }
        /* 落在 TH_DARK ~ TH_BRIGHT 之间：什么都不做，保持原状态 */

        /* ── 打印（变化超过 30 才打，避免刷屏）*/
        if (last_ao < 0 || ao > last_ao + 30 || ao < last_ao - 30 || do_val != last_do) {
            printf("AO=%4d  DO=%d  → %s  ",
                   ao, do_val, (ao < TH_DARK) ? "暗" : (ao > TH_BRIGHT ? "亮" : "中间"));
            int bars = ao * 15 / 4095;   /* 除 4095 不是 4096，满量程才画满 15 格 */
            putchar('[');
            for (int i = 0; i < 15; i++) putchar(i < bars ? '#' : ' ');
            putchar(']');
            printf("  LED %s\n", led_on ? "★亮" : "  灭");
            last_ao = ao;
            last_do = do_val;
        }

        sleep_ms(100);
    }

    return 0;
}
