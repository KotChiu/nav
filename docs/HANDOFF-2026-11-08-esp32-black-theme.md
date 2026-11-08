# ESP32 圆屏黑主题 Handoff

日期：2026-11-08

分支：`feature/board-2.1`

## 目标

这次只改 ESP32 圆屏固件 UI，不改 iPhone App 的视觉。

目标不是把界面变成纯灰阶，而是采用原仓库已有的“赛博霓虹”黑主题：

- 纯黑/近黑背景
- 白色主文字
- 青色作为主要高亮
- 品红作为强调和完成态颜色
- 天气图标、股票涨跌、来电等保留功能色

主题色参考 `firmware/boards/21/MotoNav/src/map/MapState.h` 中的主题 3：

```text
0x0021  near-black background
0x04B6  cyan
0xF815  magenta
0xB7FF  light cyan HUD
```

## 当前状态

- ESP32 设备已烧录本次固件，版本仍为 `v7.13.0-companion`。
- 串口设备：`/dev/cu.usbmodem101`
- 烧录方式：应用升级，写入 `0x10000`，不擦除 NVS。
- 本轮没有改 iPhone App 签名或安装流程。

## 主要改动

### 主题常量

`firmware/boards/21/MotoNav/src/core/DeviceContext.h`

新增统一 UI 色板：

- `C_UI_BG`
- `C_UI_PANEL`
- `C_UI_PANEL_2`
- `C_UI_TEXT`
- `C_UI_SUB`
- `C_UI_DIM`
- `C_UI_LINE`
- `C_UI_ACCENT`
- `C_UI_ACCENT2`

### 首页/时钟

`firmware/boards/21/MotoNav/src/pages/HomePage.h`

- 黑底不变。
- 外圈由灰色改为青色主题环。
- 顶部标记改为品红强调色。
- 状态/设备/版本文字改为白色、次级灰和青色高亮。
- 分页点改为白色激活、深灰未激活。

### 音乐页

`firmware/boards/21/MotoNav/src/pages/PageNavigation.h`

- 背景改为统一黑主题背景。
- 封面底环改为深色面板。
- `MUSIC`、播放状态和按钮高亮使用青色/品红。
- 按钮保持深色底、白字，不再使用大面积不同色块。

### 天气页

`firmware/boards/21/MotoNav/src/pages/WeatherPage.h`

- 页面背景统一为黑主题。
- 标题、降水柱、AQI 等使用青色高亮。
- 预警保留橙色/品红提示。
- 天气图标保留自身功能色，不强制灰阶。

### LVGL 设置/角色/股票页

`firmware/boards/21/MotoNav/src/display/LvglOverlay.h`

- 设置卡片、亮度滑杆改为黑底 + 青色指示。
- 首页快捷按钮改为黑底、青色描边、白字。
- 机器人光环改为青色，状态文字使用青色/品红。
- 股票卡片改为纯黑底、深灰描边、白色主文字。
- K 线网格使用低对比深灰，收盘参考线用品红，蜡烛保留红涨绿跌。

### 其他页面

- `PairingPage.h` 和 `NavigationPage.h` 的青色文字统一到 `C_UI_ACCENT`。
- 来电页、到达动画继续保留功能色，不再强行单色化。

## 验证

已执行并通过：

```bash
python3 firmware/build.py --board 21
python3 firmware/flash_macos.py --board 21 --port /dev/cu.usbmodem101
```

烧录结果：

- ESP32-S3 识别正常
- 应用写入 `0x10000`
- 写入数据校验通过
- 烧录后自动硬复位

## 仍需确认

- 需要看实机各页面：首页/时钟、天气、音乐、角色、股票、设置。
- 重点检查青色在低亮度下是否偏暗，以及品红强调是否过多。
- 天气图标和股票红绿是刻意保留的功能色，不需要按主题统一。
- 如果用户要求完全还原“主题 3”的地图视觉，还需要把默认地图主题切到 3；目前只统一了 UI 壳层，地图主题仍由 App 设置决定。
