# Pico 2 (RP2350) pico-sdk 学习项目

基于 Raspberry Pi Pico 2（RP2350）的学习项目

## 环境要求

| 组件            | 版本                            | 位置                                             |
| ------------- | ----------------------------- | ---------------------------------------------- |
| pico-sdk      | 2.3.1                         | `~/.pico-sdk/sdk/2.3.1/`                       |
| ARM 工具链       | GCC 15.2.1（Arm GNU 15.2.Rel1） | `~/.pico-sdk/toolchain/15_2_Rel1/`             |
| picotool      | 2.3.1                         | `~/.pico-sdk/picotool/2.3.1/picotool/picotool` |
| cmake / ninja | 4.3.4 / 1.13.2                | `~/.pico-sdk/cmake`、`~/.pico-sdk/ninja`        |

安装 pico-sdk 的官方文档：  
https://github.com/raspberrypi/pico-sdk

## 项目结构

| 目录          | 内容                              |
| ----------- | ------------------------------- |
| `00-demo/`  | 最小 Hello World，同时作为新建项目的模板，请勿删除 |
| `01-blink/` | 板载 LED（GP25）闪烁 + 串口打印计数         |
| `02-btn/`   | GP14 按键中断 + 消抖 + LED 翻转         |

## 编译 / 烧录 / 查看输出

### 使用 VS Code

1. 用 VS Code 打开**单个项目目录**，例如：
   ```bash
   code 01-blink
   ```
   不要打开仓库根目录，否则 Pico 扩展不会激活。

2. `Ctrl+Shift+P` → `Compile Pico Project` 进行编译。

3. 按住 BOOTSEL 按钮，将 Pico 2 插入 USB，然后将 `build/01-blink.uf2` 拖入出现的 RP2350 磁盘。

4. 查看串口输出（Linux/macOS）：
   ```bash
   picocom -b 115200 /dev/ttyACM0
   ```
   退出：`Ctrl+A` 然后 `Ctrl+X`。  

### 使用终端

```bash
# 设置 pico-sdk 路径（根据你的实际安装位置修改）
export PICO_SDK_PATH=/path/to/pico-sdk

cd 01-blink
cmake -B build -G Ninja -DPICO_BOARD=pico2
ninja -C build
```

## Build Type（Debug / Release）

pico-sdk 默认使用 **Release**（`-O3 -DNDEBUG`），代码小、运行快，但 `assert()` 会被关闭。  
官方扩展新建项目时默认使用 **Debug**（`-Og -g`），`assert()` 生效，报错信息更详细，但二进制更大、运行更慢。

切换方式：`Ctrl+Shift+P` → `Switch Build Type`。

> 本仓库中的 `00-demo` 和 `01-blink` 均为 **Release**。  
> 同一个 Hello World，Debug 编译出的 uf2 约 65024 字节，Release 约 44544 字节。

## 新建项目

```bash
# 在仓库根目录下执行
./new-project.sh 02-servo        # 从 00-demo 复制并自动配置 CMake
./new-project.sh --list          # 查看现有项目
```

## 官方示例库

官方 [pico-examples](https://github.com/raspberrypi/pico-examples) 包含大量示例，建议当作 API 字典查询，不要从头通读。

```bash
git clone https://github.com/raspberrypi/pico-examples.git
cd pico-examples

# 搜索某个 API 的用法
grep -rln "gpio_set_irq_enabled" --include=*.c .
grep -rn  "adc_read"             --include=*.c . | head

# 编译某一个示例（目标名要带 .elf）
export PICO_SDK_PATH=/path/to/pico-sdk
cmake -B build -G Ninja -DPICO_BOARD=pico2
ninja -C build i2c/ssd1306_i2c/ssd1306_i2c.elf
```

> 平时不建议保留 `build/` 目录（全量编译约 800 MB）。用到哪个编哪个，配置一次只需几秒。

**建议学习顺序**：  
`blink_simple` → `blink` → `hello_world` → `gpio` → `timer`/`pwm` → `uart`/`i2c`/`spi` → `adc`/`dma` → `pio`/`multicore`

**可以跳过**：  
`bluetooth`、`pico_w`（Pico 2 没有无线功能）；`encrypted`、`otp`、`bootloaders`（安全启动相关）。

## 注意

### 1. 新建项目后必须开启 USB stdio，否则 `printf` 无输出

在 `CMakeLists.txt` 中确保以下两行（官方模板默认都是 0）：

```cmake
pico_enable_stdio_uart(<项目名> 0)
pico_enable_stdio_usb(<项目名> 1)     # 改成 1
```

不开 USB stdio 时，代码能编译、能烧录，但串口没有任何输出，容易误判为硬件故障。

### 2. 必须打开单个项目目录，不能打开父目录

VS Code 的 Pico 扩展激活条件是：打开的文件夹中包含 `pico_sdk_import.cmake`。  
如果打开仓库根目录，扩展不会启动，命令面板里搜不到任何 Pico 命令，状态栏也没有按钮。
