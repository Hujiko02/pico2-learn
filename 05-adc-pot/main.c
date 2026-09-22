/* ============================================================================
 *  05-adc-pot —— 电位器 → ADC → 串口显示电压（入门第 5 课）
 *
 *  ── 接线（3 根线）──────────────────────────────────────────────────────
 *      电位器有 3 个脚，从正面看一般是「左 - 中 - 右」：
 *
 *          左脚 ──────── 3V3(OUT)   物理引脚 36
 *          中脚 ──────── GP26        物理引脚 31   ← 滑臂，读这个
 *          右脚 ──────── GND         物理引脚 38
 *
 *      中间那个脚是滑臂（wiper）。转动旋钮时它在电阻体上滑动，
 *      所以中脚上的电压会在 0V ~ 3.3V 之间平滑变化 —— 这就是「分压」。
 *
 *      ⚠️ 左右两脚接反只是方向反过来（顺时针变小还是变大），不会烧东西。
 *      ⚠️ 但【中脚千万别接 3V3 或 GND】—— 那样就没分压了，读数永远不变，
 *         你会以为是程序坏了，其实是线接错了。
 *
 *  ── 现象 ──────────────────────────────────────────────────────────────
 *      串口打印（旋钮转到哪，数字和进度条跟到哪）：
 *
 *          原始=4095  平均=4093  电压=3.299V  [##########] 100%
 *          原始=2048  平均=2047  电压=1.650V  [#####     ]  50%
 *          原始=  12  平均=  11  电压=0.009V  [          ]   0%
 *
 *      串口：picocom -b 115200 /dev/ttyACM0      （退出 Ctrl+A 松开再按 Ctrl+X）
 *
 *  ── 和 STM32 标准库 ADC 的完整对比 ───────────────────────────────────────
 *    【SPL】要 5 步 + 手工校准：
 *        1. RCC_ADCCLKConfig(RCC_PCLK2_Div6);                     配 ADC 时钟
 *           RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
 *        2. GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;         配成模拟输入
 *           GPIO_Init(GPIOA, &GPIO_InitStructure);
 *        3. ADC_InitStructure.ADC_Mode               = ADC_Mode_Independent;
 *           ADC_InitStructure.ADC_ScanConvMode       = DISABLE;
 *           ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
 *           ADC_InitStructure.ADC_ExternalTrigConv   = ADC_ExternalTrigConv_None;
 *           ADC_InitStructure.ADC_DataAlign          = ADC_DataAlign_Right;
 *           ADC_InitStructure.ADC_NbrOfChannel       = 1;
 *           ADC_Init(ADC1, &ADC_InitStructure);
 *        4. ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_55Cycles5);
 *        5. ADC_Cmd(ADC1, ENABLE);
 *           ADC_ResetCalibration(ADC1);
 *           while (ADC_GetResetCalibrationStatus(ADC1));
 *           ADC_StartCalibration(ADC1);
 *           while (ADC_GetCalibrationStatus(ADC1));      ← 忘了校准读数就是错的
 *        + 每次读还要三步：
 *              ADC_SoftwareStartConvCmd(ADC1, ENABLE);
 *              while (!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC));
 *              value = ADC_GetConversionValue(ADC1);
 *
 *    【Pico SDK】初始化 3 句 + 读 1 句：
 *        adc_init();
 *        adc_gpio_init(POT_PIN);
 *        adc_select_input(POT_CHANNEL);
 *        value = adc_read();          ← 启动 + 等待 + 取值全在里面
 *    校准也不用管，SDK 启动时就做过了。
 *
 *  ── 必须知道的 4 个限制 ─────────────────────────────────────────────────
 *    1. 【只有 4 个脚能做 ADC】，而且是写死的，没有 STM32 那种自由度：
 *           GP26 = ADC0    GP27 = ADC1    GP28 = ADC2    GP29 = ADC3
 *       （GP29 在板子上接了 VSYS 电压检测，尽量别动它）
 *    2. 【参考电压固定 3.3V】，没有 STM32 的 VREF+ 引脚可以外接基准。
 *       所以要测更准的电压得外接 ADC 芯片。
 *    3. 输入【绝对不能超过 3.3V】。要测 5V 或更高必须用电阻分压。
 *    4. 内部温度传感器占用了 ADC4，和上面 4 个通道不能同时用。
 * ========================================================================== */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"

#define POT_PIN      26     /* 电位器中脚接 GP26 */
#define POT_CHANNEL   0     /* GP26 就是 ADC0 —— 这个数字是硬件写死的 */

/* 每次打印前连读几次取平均。
 * 为什么需要：ADC 单次读数会有 ±几个字的抖动，旋钮不动时数字也在跳，
 * 看着难受。取 8 次平均后就基本稳了。
 * 【对比 SPL】STM32 里也有类似做法 —— 只是 SPL 要你自己写循环，
 * 这里也是自己写，SDK 没提供现成的平均函数。 */
#define SAMPLE_TIMES  8

/* 只有读数变化超过这个值才打印一行。
 * 不设的话旋钮不动也会每秒刷 5 行一模一样的，把有用的信息冲掉。 */
#define CHANGE_THRESHOLD  8

/* 就算读数一点没变，也至少每这么久打印一行（让你知道程序还活着）*/
#define HEARTBEAT_MS  3000

/* 文本进度条：把 0~max 映射成 10 格
 * 除数是 max 不是 max+1：value=max 时要刚好 10 格，
 * 写成 max+1 的话满量程只能画到 9 格（整数除法舍掉）。 */
static void print_bar(uint16_t value, uint16_t max) {
    int bars = (int)((uint32_t)value * 10 / max);
    if (bars > 10) bars = 10;
    putchar('[');
    for (int i = 0; i < 10; i++) putchar(i < bars ? '#' : ' ');
    putchar(']');
}

int main() {
    stdio_init_all();

    /* 等 USB 串口被打开再打印，否则开机头几行会丢（USB 还没枚举完）*/
    for (int i = 0; i < 150 && !stdio_usb_connected(); i++) sleep_ms(100);

    printf("\n");
    printf("==========================================\n");
    printf("  05-adc-pot   电位器 → ADC → 串口\n");
    printf("==========================================\n");
    printf("接线：电位器 左脚→3V3(36)  中脚→GP%d(31)  右脚→GND(38)\n", POT_PIN);
    printf("转动旋钮，下面数字应该从 0 平滑变到 4095\n");
    printf("------------------------------------------\n");

    /* ── 第 1 步：初始化 ADC 模块本身
     * 【对应 SPL】ADC_Init + ADC_Cmd + 校准，三件事合成一句 */
    adc_init();

    /* ── 第 2 步：把 GP26 配成模拟输入
     * 【对应 SPL】GPIO_Mode_AIN。
     * 这一句会同时关掉该引脚的上下拉和数字功能 —— 必须的，
     * 否则数字输入缓冲器的漏电流会影响采样精度 */
    adc_gpio_init(POT_PIN);

    /* ── 第 3 步：选中通道 0（GP26 就是通道 0）
     * 设完之后 adc_read() 读的就都是这个通道了 */
    adc_select_input(POT_CHANNEL);

    uint16_t last_printed = 0xFFFF;      /* 上次打印的平均值，用来判断要不要再打 */
    uint32_t last_print_ms = 0;

    while (true) {
        /* 连读 SAMPLE_TIMES 次取平均 —— 这就是最简单的「均值滤波」。
         * 更讲究的做法有中值滤波、滑动平均、卡尔曼滤波，入门先不管。 */
        uint32_t sum = 0;
        uint16_t raw = 0;
        for (int i = 0; i < SAMPLE_TIMES; i++) {
            raw = adc_read();            /* 12 位，0 ~ 4095 */
            sum += raw;
        }
        uint16_t avg = (uint16_t)(sum / SAMPLE_TIMES);

        /* 12 位 ADC + 3.3V 参考 → 1 个计数 = 3.3/4096 ≈ 0.000806V
         * 用 4096.0f 而不是 4095.0f：因为 4095 是最大值，
         * 满量程被分成了 4096 份。这里两种写法都有争议，
         * 差 0.02% —— 对入门项目完全无所谓。 */
        float voltage = avg * 3.3f / 4096.0f;

        uint32_t now_ms = to_ms_since_boot(get_absolute_time());

        /* 变化够大、或者超过心跳间隔，就打印一行 */
        int changed = (last_printed == 0xFFFF) ||
                      (avg > last_printed ? avg - last_printed : last_printed - avg) > CHANGE_THRESHOLD;
        if (changed || (now_ms - last_print_ms) >= HEARTBEAT_MS) {
            printf("原始=%4u  平均=%4u  电压=%.3fV  ", raw, avg, voltage);
            print_bar(avg, 4095);
            printf(" %3u%%\n", (unsigned)((uint32_t)avg * 100 / 4095));
            last_printed   = avg;
            last_print_ms  = now_ms;
        }

        sleep_ms(50);       /* 50ms 读一轮（一轮含 8 次采样）*/
    }

    return 0;
}
