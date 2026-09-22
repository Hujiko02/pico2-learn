/* ============================================================================
 *  06-buzzer —— 按键控制蜂鸣器（入门第 6 课）
 *
 *  ── 接线（5 根线）──────────────────────────────────────────────────────
 *      蜂鸣器模块是 3 个脚，看模块丝印：GND / VCC / I/O（有的标 S 或 SIG）
 *
 *          VCC ──────── 3V3(OUT)   物理引脚 36
 *          GND ──────── GND        物理引脚 38
 *          I/O ──────── GP15       物理引脚 20   ← 【低电平触发】
 *
 *      按键两脚没有方向：
 *          一脚 ──────── GP14      物理引脚 19
 *          另一脚 ────── GND       物理引脚 18
 *      （用芯片内部上拉，不用外接电阻）
 *
 *  ── 现象 ──────────────────────────────────────────────────────────────
 *      1. 上电自检：嘀 嘀 嘀 三声
 *      2. 短按按键（不到 0.5 秒）→ 嘀一声
 *      3. 按住不放（超过 0.5 秒）→ 一直响；松开停
 *      4. 串口同步打印按了多久、是短按还是长按
 *
 *  ══ ★ 本节最重要的知识点：有源蜂鸣器 vs 无源蜂鸣器 ══
 *
 *      有源（active）——你手上这个         无源（passive）
 *      ─────────────────────────────      ─────────────────────────────
 *      内部自带振荡电路                    内部只有一个线圈，没有振荡
 *      给它一个直流电平就响                必须给它【方波】才响
 *      音调固定，改不了                    方波频率 = 音调，能奏乐
 *      控制方式：GPIO 高/低                控制方式：PWM 输出方波
 *      便宜、简单                          贵一点、能做电子琴
 *
 *      怎么区分？看模块背面：
 *        · 有黑色小圆柱（带孔，像个小罐子）→ 有源
 *        · 看到裸露的线圈/绿色小板 → 无源
 *      或者：给个固定电平就响的 = 有源；不出声或者只有"嗒"一声的 = 无源。
 *
 *      ⚠️ 你这个模块是「低电平触发」——因为板上有个三极管做驱动，
 *         I/O 拉低 → 三极管导通 → 蜂鸣器响。
 *         所以下面用 BUZZER_ON() 把引脚写 0 才是响，写 1 是静音，
 *         和「GPIO 高电平点亮 LED」的直觉是反的，很容易搞混。
 *
 *  ══ 第二个知识点：什么时候用轮询，什么时候用中断 ══
 *
 *      02-btn 那个项目用的是【中断】，这里故意改成【轮询】。
 *
 *        中断适合：只关心「按了一下」这个事件，不需要知道按了多久
 *        轮询适合：需要知道按下【持续多久】（短按/长按）、或者要读多个
 *                  按键的状态组合、或者按键动作很密集
 *
 *      本项目要做短按/长按判断，必须知道按住的时长 → 用轮询更自然。
 *
 *  ══ 和 STM32 标准库的对比 ══
 *      SPL 点蜂鸣器和点 LED 是一模一样的（都是 GPIO 输出）：
 *          RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
 *          GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
 *          GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_12;
 *          GPIO_Init(GPIOB, &GPIO_InitStructure);
 *          GPIO_ResetBits(GPIOB, GPIO_Pin_12);     // 低电平 → 响
 *      Pico SDK 就少一步（时钟常开）：
 *          gpio_init(BUZZER_PIN);
 *          gpio_put(BUZZER_PIN, 1);                 // 先置静音
 *          gpio_set_dir(BUZZER_PIN, GPIO_OUT);
 *
 *      ⚠️ 注意上面三句的【顺序】：先 gpio_init（会把引脚复位的），
 *         然后 gpio_put 写静音，最后才 set_dir 成输出。
 *         如果先 set_dir 再 put，那一瞬间输出是 0，蜂鸣器会"嗒"地
 *         响一下才安静 —— 上电瞬间的一声杂音，做产品时是不允许的。
 * ========================================================================== */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define BUZZER_PIN   15     /* 蜂鸣器 I/O，低电平触发 */
#define BTN_PIN      14     /* 按键，另一端接 GND */

#define LONG_PRESS_MS   500  /* 按住超过这么久算长按 */
#define DEBOUNCE_MS      30  /* 消抖时间：机械按键抖动量级是几毫秒 */

/* 因为模块是低电平触发，所以这两句和直觉反过来。
 * 用宏包起来，以后遇到高电平触发的模块只改这里就够了。 */
#define BUZZER_ON()   gpio_put(BUZZER_PIN, 0)
#define BUZZER_OFF()  gpio_put(BUZZER_PIN, 1)

/* ── 响一声然后关掉。注意这个函数【会阻塞】，
 *    所以别在中断里调用它（中断里不能睡 100ms）。
 *    【对比 SPL】SPL 里也没有现成的 beep()，都是自己封装的。 */
static void beep(int ms) {
    BUZZER_ON();
    sleep_ms(ms);
    BUZZER_OFF();
}

/* ── 消抖后的按键读取。返回 true = 按下（低电平）
 *
 *    原理：连读 3 次，每次都间隔 DEBOUNCE_MS，三次都一样才认可。
 *    机械按键按下/松开的最初几毫秒会来回弹跳好几下，
 *    不消抖的话「按一次」会被识别成「按了好几次」。
 *
 *    【对比 SPL】STM32 教程里一般是「延时 20ms 再读一次」，
 *    原理一样，只是这里用三次取样更稳一点。 */
static bool btn_pressed(void) {
    for (int i = 0; i < 3; i++) {
        if (gpio_get(BTN_PIN)) return false;   // 读到高（未按下）直接返回
        sleep_ms(DEBOUNCE_MS);
    }
    return true;                                // 连续三次都是低 → 确实按下了
}

int main() {
    stdio_init_all();

    /* ★ 蜂鸣器先初始化。顺序很重要，见文件头的说明 */
    gpio_init(BUZZER_PIN);
    gpio_put(BUZZER_PIN, 1);            // 先写静音，避免上电瞬间"嗒"一声
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);

    /* 按键：内部上拉 */
    gpio_init(BTN_PIN);
    gpio_set_dir(BTN_PIN, GPIO_IN);
    gpio_pull_up(BTN_PIN);

    /* 等 USB 串口连上再打印 */
    for (int i = 0; i < 150 && !stdio_usb_connected(); i++) sleep_ms(100);

    printf("\n");
    printf("==========================================\n");
    printf("  06-buzzer   按键控制蜂鸣器\n");
    printf("==========================================\n");
    printf("接线：蜂鸣器 VCC→3V3(36)  GND→GND(38)  I/O→GP%d(20)\n", BUZZER_PIN);
    printf("      按键 一脚→GP%d(19)  另一脚→GND(18)\n", BTN_PIN);
    printf("------------------------------------------\n");

    /* ── 上电自检音：嘀 嘀 嘀
     * 有源蜂鸣器只能发一个音，所以「自检音」只能是节奏上的区分，
     * 做不出"哆来咪"。想奏乐得换无源蜂鸣器 + PWM。 */
    printf("自检音...\n");
    for (int i = 0; i < 3; i++) {
        beep(80);
        sleep_ms(120);
    }
    printf("自检完成。短按=嘀一声，长按=持续响\n\n");

    bool     last_pressed  = false;
    bool     is_long       = false;
    uint32_t press_start   = 0;
    int      short_count   = 0;
    int      long_count    = 0;

    while (true) {
        bool     pressed = btn_pressed();
        uint32_t now     = to_ms_since_boot(get_absolute_time());

        if (pressed && !last_pressed) {
            /* ── 刚按下：记下时刻，先不响（还不知道是长按还是短按）*/
            press_start = now;
            is_long     = false;

        } else if (pressed && last_pressed) {
            /* ── 一直按着：超过阈值就切成"长按"模式，开始持续响 */
            if (!is_long && (now - press_start) >= LONG_PRESS_MS) {
                is_long = true;
                long_count++;
                BUZZER_ON();
                printf("长按第 %d 次 → 持续响（松手停）\n", long_count);
            }

        } else if (!pressed && last_pressed) {
            /* ── 刚松开：判断这次是短按还是长按 */
            uint32_t held = now - press_start;

            if (is_long) {
                BUZZER_OFF();
                printf("  松手了，这次按住 %lu ms（长按）\n", (unsigned long)held);
            } else {
                short_count++;
                printf("短按第 %d 次 → 嘀（按住 %lu ms）\n",
                       short_count, (unsigned long)held);
                beep(100);
            }
            is_long = false;
        }

        last_pressed = pressed;
        sleep_ms(10);       /* 10ms 轮询一轮，不空转烧 CPU */
    }

    return 0;
}
