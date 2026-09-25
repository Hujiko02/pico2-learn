# 03-oled —— 软件模拟 I2C 驱动 0.96" OLED（SSD1306）

在 Pico 2（RP2350）上驱动 128×64 的 SSD1306 OLED，用的是**软件模拟 I2C**（GPIO 手动翻转 SCL/SDA），
代码移植自「江科大」的 OLED 教程，显示用的 `OLED_F8x16` 字库也是原版。

> 这份 README 重点记录了移植到 RP2350 时踩的坑：**pico2 主频太高，不加位延时这套代码点不亮**。

## 现象

上电后屏幕显示：

```
Hello Pico2!
OLED OK
12345
```

## 接线

| OLED 模块 | Pico 2 | 说明 |
| --- | --- | --- |
| GND | pin 38（或 3/8/13/18/23/28） | 地 |
| VCC | pin 36（3V3 OUT） | 用 3V3，别用 VBUS/5V |
| SCL | **pin 20 = GP15** | 时钟 |
| SDA | **pin 19 = GP14** | 数据 |

注意：
- 模块必须是 **4 脚 I2C 版**（GND/VCC/SCL/SDA）。7 脚的（GND/VCC/D0/D1/RES/DC/CS）是 SPI 版，这份代码驱动不了。
- GP14/GP15 正好是硬件 **I2C1** 的 SDA/SCL，以后想换硬件 I2C 不用改线。
- 板子用官方 Pico 2 或 **LuckFox Pico 2** 等 RP2350A 兼容板都一样（引脚顺序相同，GP14=pin19、GP15=pin20）。

## 编译 / 烧录

```bash
source ~/.pico-sdk/env.sh      # PICO_SDK_PATH / 工具链 / picotool

cd 03-oled
cmake -B build -G Ninja -DPICO_BOARD=pico2    # 首次
ninja -C build

# 进 BOOTSEL：按住 BOOT → 点一下 RESET → 松开 BOOT
picotool load -f build/03-oled.uf2 && picotool reboot -f
# 或者拖盘： cp build/03-oled.uf2 /run/media/$USER/RP2350/
```

USB stdio 在 `CMakeLists.txt` 里是关的（`pico_enable_stdio_usb(03-oled 0)`），
所以程序跑起来后电脑上**不会**出现 USB 设备，这是正常的。

## 关键代码说明

### 1. 软件 I2C 必须有位延时（本项目最重要的坑）

```c
#define OLED_DELAY_US   2

static inline void OLED_W_SCL(bool x){
    gpio_put(OLED_SCL_PIN, 0);
    gpio_set_oeover(OLED_SCL_PIN, x? GPIO_OVERRIDE_LOW : GPIO_OVERRIDE_HIGH);
    busy_wait_us_32(OLED_DELAY_US);   // ← 不能省
}
```

原因：
- RP2350 跑 150 MHz，**不加延时时 SCL 会跑到约 2.3 MHz**，而 SSD1306 的 I2C 上限是 400 kHz；
- 更致命的是这里用 **开漏**（`GPIO_OVERRIDE_LOW` = 关闭输出，引脚变高阻，靠外部上拉拉高）。
  没有延时时 SCL 高电平只有约 **100 ns**，而 4.7 kΩ 上拉 + 几十 pF 线容的上升时间要几百 ns，
  线根本升不到 VIH → SSD1306 看不到有效的时钟沿 → **整屏全黑**。

加 2 µs 后 SCL 约 90~170 kHz，稳稳在规格内。想快可以改成 `1`。

> 这套「不加延时」的代码在别人手上有时能亮（上拉阻值大、线短、或者运气好），
> 属于踩在临界线上的行为，换块板子/换根线就可能失效。别依赖它。

### 2. 关掉 SDA/SCL 的输入缓冲

```c
gpio_set_input_enabled(OLED_SCL_PIN, false);   // 本驱动只写不读
gpio_set_input_enabled(OLED_SDA_PIN, false);
```

RP2350 **A2 步进**有 E9 勘误：引脚"输入使能 + 电平处于中间态"时会漏电，
把开漏释放后的高电平钳在 ~2.2 V 左右，低于 SSD1306 的 VIH（0.8×3.3 ≈ 2.64 V），同样导致全黑。
这段驱动只写不读（连 ACK 都不读），把输入缓冲关掉即可彻底断掉这条漏电路径。

以后若要读总线（比如读 ACK），把那两行的 `false` 改回 `true` 即可。

### 3. 开漏是怎么模拟的

```c
gpio_put(pin, 0);                                                  // 输出值固定为 0
gpio_set_oeover(pin, x ? GPIO_OVERRIDE_LOW : GPIO_OVERRIDE_HIGH);  // 高阻 / 使能输出
```

`OEOVER` 的语义（SDK 注释）：`GPIO_OVERRIDE_LOW = drive low/disable output`（高阻）、
`GPIO_OVERRIDE_HIGH = drive high/enable output`（驱动器使能，输出 `gpio_put` 的值）。
对应 STM32 的 `GPIO_Mode_Out_OD`，所以**总线高电平完全依赖模块自带的 4.7 kΩ 上拉**。

## 排错清单

| 现象 | 原因 | 处理 |
| --- | --- | --- |
| 全黑不亮 | 软件 I2C 太快（无位延时） | 加 `OLED_DELAY_US`（2~5）—— 本项目就是这个问题 |
| 全黑不亮 | 模块没有上拉 / 上拉太弱 | SDA/SCL 各加 4.7 kΩ 到 3V3；或万用表量静止电压，≈3.3 V 才对 |
| 全黑不亮 | SDA/SCL 接反 | 对照接线表（GP14=SDA、GP15=SCL） |
| 全黑不亮 | 买成了 7 脚 SPI 版 | 换 4 脚 I2C 版 |
| 全黑不亮 | 从机地址不是 0x78 | 代码里写死 `0x78`（7 位 0x3C）；少数模块 SA0 拉高是 `0x7A` |
| 花屏 / 乱码 | 时序临界、上升沿太慢 | 加大延时；缩短杜邦线 |
| 偶尔亮偶尔不亮 | 开漏高电平处在临界点 | 加延时 + 加强上拉，别靠运气 |

**快速判断是"时序问题"还是"硬件问题"**：跑官方 `pico-examples/i2c/bus_scan`，
引脚改成 GP14/GP15、用 i2c1。能扫到 `0x3C` 说明接线/模块/上拉都没问题，
问题 100% 在软件的位时序上；扫不到就是电气问题（上拉、接线、模块型号）。

## 文件结构

| 文件 | 说明 |
| --- | --- |
| `main.c` | 演示：显示字符串和数字 |
| `hardware/OLED.c` | I2C 时序 + SSD1306 初始化 + 字符/数字显示 |
| `hardware/OLED_Font.h` | 8×16 ASCII 字库（原版，一行没改） |
| `hardware/OLED.h` | 对外接口 |

## CMake

`hardware/OLED.c` 是直接加进 `add_executable` 的，加新文件时记得同步 `CMakeLists.txt`：

```cmake
add_executable(03-oled
        main.c
        hardware/OLED.c
)
```

## 参考

- 江科大 STM32 入门教程（OLED 部分，软件 I2C 与字库）
- [RP2350 E9 勘误讨论](https://forums.raspberrypi.com/viewtopic.php?p=2250531)
- [LuckFox Pico 2 产品页](https://www.luckfox.cn/development-board/Pico-2) / [C 教程](https://wiki.luckfox.com/zh/Pico2/C/)
- pico-examples：`i2c/ssd1306_i2c`、`i2c/bus_scan`
