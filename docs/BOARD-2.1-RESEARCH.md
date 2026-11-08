# Waveshare ESP32-S3-Touch-LCD-2.1 调研与移植状态

## 结论

可以在 `coolcoolmanman-design/MotoNav` 上继续开发。硬件与项目现有架构兼容，主要差异不是分辨率，而是显示接口：

- 1.75 屏：QSPI AMOLED，驱动 `CO5300`，逻辑和物理分辨率都是 `466x466`
- 2.1 屏：3-wire SPI 控制 + 16-bit RGB 显示，驱动 `ST7701S`，物理分辨率 `480x480`
- 2.1 屏触摸：`CST820`，I2C 地址 `0x15`；1.75 屏使用 `CST92xx`
- 2.1 屏使用 `TCA9554PWR` 扩展 IO 控制 LCD reset、touch reset 和 3-wire SPI CS

结论：能移植，手机端协议不需要改。第一版固件目标是点亮屏幕、显示现有界面、响应触摸，并在串口确认 `MotoNav ready`。

## 当前状态

工作分支：`feature/board-2.1`

### 页面与手势（开发中）

页面采用单一横向顺序：主页（导航、时钟和天气摘要合并）、天气、音乐、互动角色、股票、设置。左滑前进、右滑返回，下滑统一回主页；边界不循环。轻划超过 48 个逻辑像素即可切换。

手动换页使用页面快照跟随手指，松手后短动画完成或回弹；股票页的纵向手势交给 LVGL 原生滚动。90°整数旋转采用直接像素寻址，任意角度仍使用重采样。顶部 LVGL 状态栏显示电量；界面以黑色为主色，最多 8 只股票卡片在纵向列表中滚动，中文名由手机渲染成位图，卡片内显示 12 日缩略 K 线。实机流畅度仍需验证。

已完成：

- 复制 1.75 固件作为板级基线到 `firmware/boards/21/MotoNav`
- 添加 `Board21Panel.h`，直接初始化 `ST7701S`、`TCA9554PWR`、RGB DMA 和背光
- 添加 `PortScale.h`，把现有 `466x466` 逻辑画布映射到 `480x480`
- 使用仓库内 `SensorLib` 的 `CST816/CST820` 驱动，并把触摸坐标映射到逻辑画布
- 添加基于 GPIO4 ADC 的电池电压估算
- 让 `firmware/build.py` 和 `firmware/build.ps1` 支持 `-Board 21` / `--board 21`
- 已烧录到实物，串口确认 `touch ok` 和 `MotoNav ready`

本地编译结果：

```text
Sketch uses 1228866 bytes (39%) of program storage space.
Global variables use 93924 bytes (28%) of dynamic memory.
```

## 构建

macOS / Linux：

```bash
python3 firmware/build.py --board 21
```

Windows PowerShell：

```powershell
.\firmware\build.ps1 -Board 21
```

构建产物位于 `firmware/dist/21/panel0`。首次完整安装使用 `full_0x0.bin`，已有相同分区的升级使用 `app_0x10000.bin`。不要将 1.75 或 1.85B 固件刷到 2.1 屏，也不要反向混刷。

## 实机验证前注意事项

将开发板用数据线接到电脑后，先执行：

```bash
arduino-cli board list
```

串口通常形如 `/dev/cu.usbmodem*`。烧录和串口输出可用：

```bash
python3 firmware/flash_macos.py --board 21 --check
python3 firmware/flash_macos.py --board 21 --full
```

当前已完成：

1. `ST7701S` 初始化成功，串口出现 `touch ok` 和 `MotoNav ready`
2. 固件重新烧录后不再出现 I2C 驱动冲突或重启循环

仍需要继续验证：

- 屏幕画面是否居中、颜色和方向是否正确
- 触摸点按、滑动换页和来电按住行为
- `CST820` 坐标是否需要镜像或交换
- RGB DMA 的实际刷新帧率和低 PSRAM 表现
- GPIO4 电压读数与电池实际电压相符
- 外接电源检测和 5 分钟闲置关机策略
- QMI8658 六轴传感器是否启用和标定
- SD 卡、RTC、蜂鸣器以及完整手机端 BLE 导航回归

## 参考

- [Waveshare 官方文档](https://docs.waveshare.net/ESP32-S3-Touch-LCD-2.1/)
- [Waveshare 官方示例包](https://files.waveshare.net/wiki/ESP32-S3-Touch-LCD-2.1/ESP32-S3-Touch-LCD-2.1-Demo.zip)
