# Pico 2 (RP2350) C/C++ 项目

## 环境（已配好，不用再装）

| 组件 | 版本 | 位置 |
|---|---|---|
| pico-sdk | 2.3.1 | `~/.pico-sdk/sdk/2.3.1/` |
| ARM 工具链 | GCC 15.2.1（Arm GNU 15.2.Rel1）| `~/.pico-sdk/toolchain/15_2_Rel1/` |
| picotool | 2.3.1 | `~/.pico-sdk/picotool/2.3.1/picotool/picotool` |
| cmake / ninja | 4.3.4 / 1.13.2 | `~/.pico-sdk/cmake`、`~/.pico-sdk/ninja`（另有 apt 的同版本）|

## 项目

| 目录 | 内容 |
|---|---|
| `00-demo/` | 最小 Hello World（同时也是建新项目的模板，别删）|
| `01-blink/` | 板载 LED（GP25）闪烁 + 串口打印计数 |
| `02-btn/` | GP14 按键中断（对应 SPL 的 EXTI）+ 消抖 + LED 翻转 |

## 编译 / 烧录 / 看输出

```
1. code ~/Projects/pico2-new/01-blink          打开【单个项目】，不要打开上层目录
2. Ctrl+Shift+P → Compile Pico Project          编译
3. 按住 BOOTSEL 插 USB → 拖 build/01-blink.uf2 进 RP2350 盘   烧录
4. picocom -b 115200 /dev/ttyACM0               看 printf 输出（退出 Ctrl+A 再 Ctrl+X）
```

终端里编译也行：

```bash
cd ~/Projects/pico2-new/01-blink
source ~/.pico-sdk/env.sh
cmake -B build -G Ninja -DPICO_BOARD=pico2 && ninja -C build
```

## Build Type（Debug / Release）

pico-sdk 不指定时默认 **Release**（`-O3 -DNDEBUG`，代码小、跑得快、`assert()` 被关掉）。
官方扩展新建项目时会写成 **Debug**（`-Og -g`，`assert()` 生效，报错信息更详细，但二迳制大、跑得慢）。

切换：`Ctrl+Shift+P` → `Switch Build Type`

> 本目录里 `00-demo` 和 `01-blink` 都是 **Release**。
> 同一个 Hello World，Debug 编出来 uf2 = 65024 字节，Release = 44544 字节，差在这里。

## 建新项目

```bash
cd ~/Projects/pico2-new
./new-project.sh 02-servo        # 从 00-demo 复制，自动配好 CMake
./new-project.sh --list          # 看现有项目
```

## 官方例子库 `~/Projects/pico-examples`

官方 pico-examples 全量源码，39 个分类 / 200 个 `.c` 文件。
**当 API 字典查，不要从头读** —— SDK 文档很薄，例子才是真相。

```bash
cd ~/Projects/pico-examples

# ① 最有用的一招：搜某个 API 别人怎么用的
grep -rln "gpio_set_irq_enabled" --include=*.c .
grep -rn  "adc_read"             --include=*.c . | head

# ② 编译某一个例子（目标名要带 .elf）
source ~/.pico-sdk/env.sh
cmake -B build -G Ninja -DPICO_BOARD=pico2      # 3 秒
ninja -C build i2c/ssd1306_i2c/ssd1306_i2c.elf  # 只编这一个
```

> 平时**不保留** `build/`（全编要 ~800 MB）。用到哪个编哪个，配置只要 3 秒。

**建议顺序**：`blink_simple` → `blink` → `hello_world` → `gpio` → `timer`/`pwm`
→ `uart`/`i2c`/`spi` → `adc`/`dma` → `pio`/`multicore`

**可以跳过**：`bluetooth`(60) `pico_w`(24) —— Pico 2 没有无线；`encrypted`/`otp`/`bootloaders` —— 安全启动

## ⚠️ 两个必知的坑

### 1. 新建项目后要开 USB stdio，否则 printf 没有任何输出

`CMakeLists.txt` 里这两行，官方模板默认**都是 0**：

```cmake
pico_enable_stdio_uart(<项目名> 0)
pico_enable_stdio_usb(<项目名> 1)     # ← 改成 1
```

不开的话代码能编过、能烧进去，但串口一片空白，很容易误判成板子坏了。

> 本目录里 `00-demo` 和 `01-blink` 都已经改成 1 了，`new-project.sh` 复制出来的也是开着的。

### 2. 必须打开单个项目，不能打开父目录

官方扩展的激活条件是「打开的文件夹里有 `pico_sdk_import.cmake`」。
打开 `~/Projects/pico2-new` 这个父目录的话，扩展根本不启动，
命令面板里搜不到任何 Pico 命令，状态栏也没有按钮。

```
✅ code ~/Projects/pico2-new/01-blink
❌ code ~/Projects/pico2-new
```

## 排错

| 现象 | 原因 / 解决 |
|---|---|
| 「找不到此文件的编译信息」 | 项目还没配置过 CMake。`Ctrl+Shift+P` → `Configure CMake`（或 `Compile Pico Project`）|
| 命令面板里没有 Pico 命令 | 打开了父目录，改成打开单个项目 |
| 串口 Permission denied | udev 规则应该已配好；拔插一次 USB，或 `sudo udevadm control --reload-rules` |
| `Import "pico/stdlib.h"` 有红波浪线 | 同上第一条，配置一次 CMake 就好 |

## 环境升级

```bash
pico-upgrade --check        # 看有没有新版 SDK
pico-upgrade <版本号>       # 升级（会自动带配套工具链并改掉各处版本号）
```
