/**
 * Pico 2 (RP2350) 使用 PWM 驱动舵机 - Pico SDK 完整示例
 *
 * 硬件连接：
 *   舵机信号线（橙/黄） -> GP15
 *   舵机电源（红）      -> 外部 5V 电源正极（不要直接用 Pico VBUS 带大舵机）
 *   舵机地（棕/黑）     -> 外部 5V 电源负极，同时与 Pico GND 共地
 *
 * PWM 参数：
 *   频率：50Hz（周期 20ms = 20000us）
 *   脉冲：0.5ms ~ 2.5ms 对应 0° ~ 180°
 *
 * 原理：
 *   将 PWM 时钟设为 1MHz，即每个计数代表 1us。
 *   wrap = 20000 - 1 = 19999，所以 PWM 周期 = 20000us = 20ms，频率 50Hz。
 *   设置通道比较值 level 为 500~2500，即可输出 0.5ms~2.5ms 的高电平脉冲。
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

// ==================== 用户配置 ====================

// 舵机信号线连接的 GPIO
#define SERVO_GPIO 15

// 舵机 PWM 频率：50Hz
#define SERVO_PWM_FREQ 50

// PWM 周期（单位：us）：1/50Hz = 20ms = 20000us
#define SERVO_PWM_PERIOD_US 20000

// 为了简化计算，将 PWM 时钟设置为 1MHz，即每个计数 1us
#define SERVO_PWM_CLK_HZ 1000000

// PWM 计数上限 wrap：周期计数 - 1
#define SERVO_PWM_WRAP (SERVO_PWM_PERIOD_US - 1)   // 19999

// 舵机脉冲宽度范围（单位：us）
#define SERVO_MIN_PULSE_US 500    // 0°
#define SERVO_MAX_PULSE_US 2500   // 180°

// ==================== 全局变量 ====================

// 保存 PWM 切片号和通道号，方便后续设置角度
static uint servo_slice;
static uint servo_chan;

// ==================== 函数实现 ====================

/**
 * @brief 初始化舵机 PWM
 * @param gpio 舵机信号线连接的 GPIO 编号
 */
void servo_init(uint gpio) {
    // 1. 将指定 GPIO 功能设置为 PWM 输出
    gpio_set_function(gpio, GPIO_FUNC_PWM);

    // 2. 获取该 GPIO 对应的 PWM 切片号（slice）和通道号（channel）
    //    RP2040/RP2350 每个 PWM 切片有两个通道，共用同一个计数器
    servo_slice = pwm_gpio_to_slice_num(gpio);
    servo_chan  = pwm_gpio_to_channel(gpio);

    // 3. 获取 PWM 默认配置结构体
    pwm_config config = pwm_get_default_config();

    // 4. 计算时钟分频值，使 PWM 时钟 = 1MHz
    //    系统时钟 clk_sys 在 Pico 2 上默认 150MHz，Pico 1 上默认 125MHz
    //    使用 clock_get_hz 动态获取，保证代码通用
    uint32_t sys_clk = clock_get_hz(clk_sys);
    float clkdiv = (float)sys_clk / (float)SERVO_PWM_CLK_HZ;

    // 设置分频系数（支持小数分频，SDK 内部会转换为 8.4 格式）
    pwm_config_set_clkdiv(&config, clkdiv);

    // 5. 设置计数上限 wrap
    //    PWM 频率 = PWM时钟 / (wrap + 1)
    //    这里 PWM时钟 = 1MHz，wrap + 1 = 20000，所以频率 = 50Hz
    pwm_config_set_wrap(&config, SERVO_PWM_WRAP);

    // 6. 初始化 PWM 切片，并立即启用
    //    最后一个参数 true 表示初始化后马上启动 PWM
    pwm_init(servo_slice, &config, true);
}

/**
 * @brief 设置舵机角度
 * @param angle 目标角度，范围 0.0 ~ 180.0
 */
void servo_set_angle(float angle) {
    // 限制角度范围，避免舵机堵转损坏齿轮
    if (angle < 0.0f)   angle = 0.0f;
    if (angle > 180.0f) angle = 180.0f;

    // 将角度线性映射到脉冲宽度（单位：us）
    // pulse_us = 500 + (angle / 180) * (2500 - 500)
    float pulse_us = SERVO_MIN_PULSE_US +
                     (angle / 180.0f) * (SERVO_MAX_PULSE_US - SERVO_MIN_PULSE_US);

    // 因为 PWM 时钟为 1MHz，每个计数 = 1us，所以 level 直接等于 pulse_us
    // 注意：level 不能超过 wrap，但 2500 < 19999，安全
    uint16_t level = (uint16_t)pulse_us;

    // 设置 PWM 通道比较值，即高电平持续时间
    pwm_set_chan_level(servo_slice, servo_chan, level);
}

/**
 * @brief 主函数
 */
int main() {
    // 初始化标准输入输出，方便调试（可选）
    stdio_init_all();

    // 初始化舵机 PWM，信号引脚使用 GP15
    servo_init(SERVO_GPIO);

    // 主循环：让舵机在 0° 和 180° 之间往复摆动
    while (true) {
        // 从 0° 缓慢转到 180°
        for (int angle = 0; angle <= 180; angle += 1) {
            servo_set_angle((float)angle);
            sleep_ms(5);   // 等待舵机转到指定角度，数值越小转动越快
        }

        // 从 180° 缓慢转到 0°
        for (int angle = 180; angle >= 0; angle -= 1) {
            servo_set_angle((float)angle);
            sleep_ms(5);
        }
    }

    return 0;
}
