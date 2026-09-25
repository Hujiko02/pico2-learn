#include "pico/stdlib.h"
#include "OLED.h"

int main()
{

    OLED_Init();            // 初始化 OLED
    OLED_ShowString(1, 1, "Hello Pico2!");   // 第1行第1列
    OLED_ShowString(2, 1, "OLED OK");
    OLED_ShowNum(3, 1, 12345, 5);

    while (true)
    {
        // 主循环
    }
}
