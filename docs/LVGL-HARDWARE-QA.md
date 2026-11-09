# MotoNav 2.1 圆屏：一键烧录后自动验收

> **只给 Waveshare ESP32-S3-Touch-LCD-2.1 / ST7701S 480×480 使用。**
> LVGL 迁移 PR #2 中提供单独的 **QA_AUTOTEST** 测试固件。不依赖联网、不格式化存储区、不擦除照片、不会修改已保存的旋转/亮度。用户暂时没有硬件，也可以预先完成 QA 固件编译；它是否在真实设备上通过，必须在烧录后才知道。

## 设计原则

- 普通固件默认 **QA 关闭**，启动速度与导航逻辑不受影响。
- 单独的 `--qa` 固件在**每一次开机**（包含烧录后的第一次重启）自动跑自检。运行期间尚未启动 BLE 服务，不与实时导航争抢屏幕和蓝牙。
- 自检完成后，恢复用户之前保存的屏幕角度，并继续启动正常 BLE/导航功能。USB 可以稍后再连接：最新的 JSON 自检摘要保存在 Preferences 命名空间 `motonavqa`。
- 屏幕无法回传肉眼看到的颜色和箭头朝向。软件只会检查 RGB565 内存读写、LVGL 绘图调用和刷新耗时；**实际颜色、反向、触摸对齐和可读性仍需要看屏幕确认**。
- **QA 固件不用于行车**：每次重启先显示色块与旋转测试，之后才开放导航。正式版需要换回普通固件。

## QA 烧录版本准备（Mac）

安装 Arduino CLI 与 ESP32 Core 3.3.10，使用仓库已有工具编译：

```bash
python3 firmware/build.py --board 21 --jobs 2 --raster-surface native --qa
```

构建会独立生成 `firmware/dist/21/panel0/qa-autotest/` 下的 `*_QA_AUTOTEST_app_0x10000.bin`、`*_QA_AUTOTEST_full_0x0.bin` 和 SHA-256 清单，不覆盖原正式固件。PR #2 的 Actions 也提供名为 `MotoNav-2.1-QA-AUTOTEST-NOT-DEVICE-TESTED` 的附件。

烧录前先只检查设备与镜像：

```bash
python3 firmware/flash_macos.py --board 21 --qa --check
```

若这块 2.1 圆屏已经安装了**相同 16 MB / app3M_fat9M_16MB 分区配置**的 MotoNav：

```bash
python3 firmware/flash_macos.py --board 21 --qa
```

如果是出厂空白设备，需要完整刷写并确认，可能覆盖已有闪存数据：

```bash
python3 firmware/flash_macos.py --board 21 --qa --full
```

**不要将 QA 固件烧录到 1.75、1.85 或其它开发板。** 如果从 Actions 下载文件，请把三种文件（两个镜像和一个 manifest）完整放到对应 `qa-autotest` 目录。刷写脚本验证板型、应用偏移、长度及 SHA-256，`--qa` 是必填的明确选择。

## 烧录后会自动运行的检查

| 过程 | 自动检测 | 屏幕/交互需要核对 |
|---|---|---|
| PSRAM | 分配 16 KiB 写读验证；记录空闲容量 | 无 |
| RGB565 主 framebuffer | 指针、像素长度、5 种色彩图案的内存回读 | 红、绿、蓝、白、黑是否真实显示且颜色准确 |
| RGB 双缓冲 | 备用帧、刷新信号量存在 | 是否有黑屏、花屏 |
| LVGL 初始化 | LVGL display 指针、状态 | 文字和圆环位置 |
| 帧率/刷新 | 测量同步刷屏中最慢一次耗时（基准阈值 500 ms） | 颜色轮换是否卡顿或闪屏 |
| 旋转 | 自动显示 **0°、90°、314°** 三种方向测试图，结束后恢复原设置 | 画面方向和边缘比例是否正确 |
| LVGL Map Canvas | 初始化独立 PSRAM、绘制地图区域多边形和道路折线 | 线条、色块、路线有无错位 |
| 显示缓冲隔离 | 地图图像源、主绘制源、DMA 提交缓冲不别名 | 无 |
| 手指触摸 | 6.5 秒内四个黄点，成功点中自动变绿并记录坐标 | 请用手指依次点四个位置；没点击记为 MANUAL，不算硬件坏 |
| 最终内存 | 剩余 PSRAM 大于 256 KiB | 无 |
| 真实蓝牙/导航 | 开机后被动观察是否已认证连接、收到导航、进入地图，并打印 OBSERVED | 必须连接手机 App 才能看到；不能在离线测试里伪造成功 |

> 自动报告里的 `PASS` 只表示对应的**软件或内存自检**通过；`MANUAL` 表示需要人眼/触摸核对。物理屏的像素信号、DMA 硬件寿命、触摸对齐、运动中导航提示并没有完全自动验证。

## 看结果（即使错过开机日志）

用 USB 数据线连接圆屏。安装 `pyserial`：

```bash
python3 -m pip install pyserial
python3 tools/hardware-qa/collect.py
```

如果电脑检测出多台串口设备，则明确指定：

```bash
python3 tools/hardware-qa/collect.py --port /dev/cu.usbmodemXXXX
```

程序发 `QAREPORT` 指令读取 NVS 内最后一次报告，生成本地 `motonav-qa-report.json`，例如：

```json
{
  "schema": 1,
  "firmware": "v7.13.0-companion",
  "run": 1,
  "pass": 8,
  "fail": 0,
  "manual": 2,
  "total": 10,
  "touch_dots": 4,
  "flush_worst_us": 21000,
  "psram_start": 5200000,
  "psram_end": 4700000,
  "optical_verified": false
}
```

上面的数字**仅示意报告结构，不是真机测量结果**。串口输出每个过程为 `QA,<测试名>,PASS/FAIL/MANUAL,<数值>`，以 `QA_BEGIN` 和 `QA_END` 作为界限。

手工重新跑一次测试（先停止导航、断开手机 BLE，确保不在来电、动画或滑动状态）：

```bash
python3 tools/hardware-qa/collect.py --run
```

`QARUN` 在有 BLE 连接或正导航时会返回 `QA_REJECT,BUSY`，不会打断实时导航。QA 镜像同时每 10 秒记录一次 `QA_RUNTIME,telemetry`，用于观察 PSRAM 与导航状态；只有真实手机连接才会出现 `BLE_AUTH`、`NAVIGATION`、`MAP_STREAM` 的 `OBSERVED`。

## 怎么让我协助定位问题

把 `motonav-qa-report.json`、串口中 `QA,` / `QA_RUNTIME,` 开头的日志，以及拍摄的一小段测试色块/旋转/四角触摸视频发到聊天。可以对照这三份证据逐项定位：是物理屏幕颜色、内存/刷新、转向角度、触摸坐标还是 BLE 地图协议出了问题。

## 回到正常固件

```bash
python3 firmware/build.py --board 21 --raster-surface native
python3 firmware/flash_macos.py --board 21 --check
python3 firmware/flash_macos.py --board 21
```

普通固件**不会在开机运行 QA**；保存的 `motonavqa` NVS 结果不影响正式界面。实际设备的自动测试尚未运行，必须在你烧录后才能报告硬件成功。
