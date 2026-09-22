#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

// 板载 LED 在 GPIO 25
#define LED_PIN 25

// PWM 频率，1000Hz 足够高，肉眼看不出闪烁
#define PWM_FREQ 1000

// PWM 计数上限，占空比范围是 0 ~ PWM_WRAP
#define PWM_WRAP 1000

// 每步延时和步进值，决定呼吸速度
#define STEP_DELAY_MS 5
#define STEP 5

int main() {
    // 1. 把 GPIO 25 设置为 PWM 功能
    gpio_set_function(LED_PIN, GPIO_FUNC_PWM);

    // 2. 获取 GPIO 25 对应的 PWM 切片和通道
    uint slice = pwm_gpio_to_slice_num(LED_PIN);
    uint chan  = pwm_gpio_to_channel(LED_PIN);

    // 3. 计算分频系数，让 PWM 频率等于 PWM_FREQ
    //    频率 = 系统时钟 / (clkdiv * (wrap + 1))
    float clkdiv = (float)clock_get_hz(clk_sys) / (PWM_FREQ * (PWM_WRAP + 1));
    pwm_set_clkdiv(slice, clkdiv);

    // 4. 设置计数上限
    pwm_set_wrap(slice, PWM_WRAP);

    // 5. 初始占空比为 0
    pwm_set_chan_level(slice, chan, 0);

    // 6. 使能 PWM
    pwm_set_enabled(slice, true);

    int level = 0;      // 当前占空比
    int dir = 1;        // 1 表示变亮，-1 表示变暗

    while (true) {
        // 更新占空比
        pwm_set_chan_level(slice, chan, level);

        // 累加步进
        level += dir * STEP;

        // 到达最亮，开始变暗
        if (level >= PWM_WRAP) {
            level = PWM_WRAP;
            dir = -1;
        }

        // 到达最暗，开始变亮
        if (level <= 0) {
            level = 0;
            dir = 1;
        }

        sleep_ms(STEP_DELAY_MS);
    }

    return 0;
}
