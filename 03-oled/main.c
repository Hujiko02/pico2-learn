#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "ssd1306.h"

int main(void) {
    stdio_init_all();
    sleep_ms(3000);

    printf("OLED test start\n");

    // I2C1: GP2=SDA, GP3=SCL
    i2c_init(i2c1, 100000);
    gpio_set_function(2, GPIO_FUNC_I2C);
    gpio_set_function(3, GPIO_FUNC_I2C);
    gpio_pull_up(2);
    gpio_pull_up(3);

    // 直接向 0x3C 写一个命令，用超时版本，不会卡死
    uint8_t cmd = 0xAE;   // 关显示命令，随便发一个
    int ret = i2c_write_timeout_us(i2c1, 0x3C, &cmd, 1, false, 10000); // 10ms 超时

    if (ret < 0) {
        printf("NO ACK from 0x3C, ret=%d\n", ret);
        printf("Check: SDA/SCL wiring, VCC, GND, address\n");
        while (1) {
            printf("no ack loop\n");
            sleep_ms(1000);
        }
    }

    printf("ACK from 0x3C, screen detected\n");

    ssd1306_t disp;
    disp.external_vcc = false;

    if (!ssd1306_init(&disp, 128, 64, 0x3C, i2c1)) {
        printf("ssd1306_init failed\n");
        while (1) sleep_ms(1000);
    }

    printf("init ok, drawing...\n");

    ssd1306_clear(&disp);
    ssd1306_draw_string(&disp, 0, 0, 1, "OLED OK");
    ssd1306_draw_string(&disp, 0, 16, 1, "Hello World");
    ssd1306_show(&disp);

    printf("display updated\n");

    while (1) {
        printf("alive\n");
        sleep_ms(1000);
    }
}