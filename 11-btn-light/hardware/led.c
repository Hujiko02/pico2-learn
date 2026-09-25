#include "pico/stdlib.h"
#define LED1_PIN 0
#define LED2_PIN 2

void led_init(void)
{
    gpio_init(LED1_PIN);
    gpio_set_dir(LED1_PIN, GPIO_OUT);
    gpio_put(LED1_PIN, 0);

    gpio_init(LED2_PIN);
    gpio_set_dir(LED2_PIN, GPIO_OUT);
    gpio_put(LED2_PIN, 0);
}

void led1_on(void)
{
    gpio_put(LED1_PIN, 1);
}

void led1_off(void)
{
    gpio_put(LED1_PIN, 0);
}

void led2_on(void)
{
    gpio_put(LED2_PIN, 1);
}

void led2_off(void)
{
    gpio_put(LED2_PIN, 0);
}

void led1_toggle(void)
{
    gpio_xor_mask(1u << LED1_PIN);
}

void led2_toggle(void)
{
    gpio_xor_mask(1u << LED2_PIN); 
}