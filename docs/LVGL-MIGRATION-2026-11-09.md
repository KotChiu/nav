# MotoNav LVGL 渐进迁移记录 · 2026-11-09

分支：`codex/lvgl-desktop-migration-20261109`，目标：2.1 英寸圆屏。**本分支为实验 Draft，未合并、未烧录真机。**

## 当前架构

手机通过 BLE 提供导航状态、文字位图、天气与封面；ESP32 继续用 Arduino GFX/Canvas 生成尚未迁移的地图等画面。LVGL 8.3.10 已用于主页环、天气摘要、音乐状态、设置/股票/角色等控件。

ST7701S RGB 显示的初始化、DMA、VSYNC、背光、480×480 物理缓冲由板级驱动负责；LVGL 管的是 UI，不应直接删除底层 RGB 驱动。

## 迁移清单

| 模块 | 目前迁移状态 | 未迁移及验证 |
| --- | --- | --- |
| 主页环 | `ui/LvglHomeChrome.h`；桌面和固件共用 | 须用实机对比旧 Canvas 外观 |
| 天气摘要 | `ui/LvglWeatherView.h`；固件接真实 BLE 字段 | 图标等视觉元素未完全还原 |
| 音乐状态 | `ui/LvglMusicStatusView.h`；固件接 HID/AMS 等真实状态 | 媒体控件和 Android/iOS 事件回归 |
| 中文天气/音乐与歌词位图 | `ui/LvglRasterAssets.h` 将灰度位图转换成 LVGL image，已有槽位 3/4/5/6/12 | 资源分配失败时保留 Canvas 回退；实机 PSRAM 验证 |
| **本轮：天气详情中文位图** | 新接入槽位 7/8（降雨/空气），9/10/11（预警）及对应 tint/y 位置 | 降雨条图、预警说明仍由 Canvas 画；无字图/空字图/切换详情需要实测 |
| 音乐封面 | `MotoNavCoverAsset` 支持圆形 RGB565 封面及旧路径回退 | 实机内存、缩放、层叠/透明 |
| 导航地图与 HUD | `ui/LvglNavigationHud.h`：速度、限速、摄像头距离为共享原生 LVGL；`LvglMapCanvas.h`：地图栅格 RGB565、道路、多边形及路线在专用 PSRAM Canvas 上绘制，安全范围内采用 LVGL `draw_polygon/draw_line` | 复杂线条、Vice City 抗锯齿、栅格采样、车道/交通灯等仍复用旧算法；PSRAM 不足或交通灯覆盖时自动退回旧绘制路径 |
| 滑动切页 | `LvglPageSlide.h` + `TransitionPolicy.h`：非导航的主页/天气/音乐/设置/互动角色/股票复用两张 PSRAM 快照，通过 LVGL Image 平移动画 | 导航中保留旧合成器（`MOTONAV_NATIVE_NAV_SLIDES=0`），需实机测试触摸、旋转及刷新性能 |

## 本轮变更

- `LvglOverlay.h`：共享 LVGL 位图资产由 5 个扩为 10 个，覆盖手机推送的天气详情中文字图。根据 `wxDetail` 和当前页面显示相应槽位，不在其他页面泄露。
- `WeatherPage.h`：天气详情页优先展示已经准备好的 LVGL 字图，未准备好时沿用 Canvas 画法。QWeather 原始雨量柱状图、提醒文本保留。
- `TextBitmapReceiver.h`：槽位 7–11 收到新图或清除时，同步标记天气页面 dirty；拒绝分片号越界、空 payload、错误 offset/超出图片总长及空接收缓冲，避免非法位移与越界写入；保持手机 BLE 包格式不变。
- `RasterBridgeTest.cpp`：单元测试 4-bit 灰度转换、透明像素、裁剪尺寸、专辑封面圆形裁切；新增 GitHub Actions 运行命令。

## 本轮后续变更：导航 HUD 与原生页面滑动

- `LvglNavigationHud.h` 使用 LVGL label、圆形限速牌与动态颜色，接入 `LvglOverlay.h`；`MapFrame.h` 在 LVGL 已启动时不再重复绘制旧速度 HUD，未启动时保留 Canvas 回退。状态包括实时车速、超速颜色、限速及摄像头距离。
- `MapState.h` 通过 accessor 把白天/夜间地图主题色提供给 LVGL 层，避免把地图内部宏硬编码进先于地图初始化的头文件中。
- `ui/MapProjection.h` 提取地图世界坐标到 466×466 逻辑画布的旋转/缩放变换；`MapFrame.h`、`RouteRenderer.h`、`RadarEffects.h` 已复用相同计算，桌面导航示例也共用此函数。`MapProjectionTest.cpp` 验证不同方向/比例下与旧 Canvas 数学公式一致，**尚未替换 Canvas 地图填充和画线 API**。
- `LvglPageSlide.h` + `PageSlideGeometry.h` 解耦滑动几何计算与显示实现；最初仅设置/互动角色/股票，随后扩展到**所有非导航状态下的六个页面**。动画仍复用原来的两张 PSRAM 快照；导航进行时的切页受 `MOTONAV_NATIVE_NAV_SLIDES=0` 保护。
- `LvglOverlay.h` 保持地图正常的异步刷新节奏；LVGL HUD 更新不会强制把地图变成同步刷新。
- 桌面模拟器在导航预览中使用相同 `LvglNavigationHud`，并接入原生滑动层；数据及道路仍是模拟的。
- `PageSlideGeometryTest.cpp` 验证正反方向与横竖滑动全过程无缝覆盖；SDL2 冒烟检查创建导航 HUD 和原生滑动层。

## 本轮：地图图层、导航提示与缓冲所有权

- `ui/LvglMapCanvas.h`：**新的地图显示容器已真正接入 2.1 固件**。在 LVGL 初始化后、进入地图模式时，按需申请一块独立的 466×466 RGB565 PSRAM 图像源（434312 字节），与主 LVGL 绘制缓冲、DMA 快照明确分离。地图原有栅格采样器仍写入这一源缓冲，由 LVGL Canvas 负责合成到屏幕；尚不是栅格算法的完全原生重写。
- `MapPrimitives.h`：道路/导航路线的折线，以及公园、水面的多边形，在点数不超过 96、坐标有限且位于安全边界内时，使用 **LVGL 8.3 `lv_canvas_draw_line/polygon`** 直接绘制。大型路网、超界多边形、Vice City 特殊抗锯齿仍自动回退到旧 Canvas 绘制，但输出进入同一 LVGL 地图图层。
- `MapFrame.h`：开始地图帧时临时切换绘制目标，绘完立即恢复主逻辑 framebuffer，再通知 LVGL 图层失效重绘。尚未迁移的交通灯提示可能覆盖地图，因此检测到交通灯覆盖时本帧自动使用旧路径。低 PSRAM、来电、配对、滑动和异常情况也会回退。不会让 PSRAM 地图图片与已提交的 RGB DMA 源互相别名。
- `ui/LvglNavigationCues.h` + `TurnCueGeometry.h`：新实现原生 LVGL 转向箭头/方向与环形进度显示，Mac 模拟器可预览；**固件默认 `MOTONAV_NATIVE_NAV_CUES=0`，真实导航依旧使用旧箭头和进度环**，待实机验证再开放开关。复杂 U 型转弯/终点样式仍有旧路由兜底，不能当作所有箭头已迁移。
- `ui/TransitionPolicy.h`：非导航时六个页面间的滑动已走 LVGL 图片合成；**导航时仍用原来的逐像素滑动代码**，以免车辆运行中丢失提示。
- `ui/LvglRasterSurface.h` + `Rgb565Plane.h`：主绘制表面可改为不依赖 `Arduino_Canvas` 的 `Arduino_GFX` 兼容软件表面，直接管理 RGB565 PSRAM；行/列/矩形裁剪有独立 Host 测试。此处保留旧绘图接口用于尚未完全迁移的模块，**尚不等于全量原生 LVGL UI**。
- `firmware/build.py --board 21 --raster-surface native|legacy`：通过宏切换新绘制表面或原 Canvas 后端，CI 同时编译两个变体以确保回退路径不失效。
- `ui/FrameOwnership.h` + `DisplayPipeline.h`：明确主逻辑绘制、LVGL Map 源、RGB flush 快照的职责。PSRAM/信号量/任务创建失败时退回同步刷新；VSYNC 等待超时不再无限阻塞，而是记录错误后重启。实际设备若 VSYNC 异常，可能重复重启，**必须实机测试**。
- 新增纯 C++ `FrameOwnershipTest.cpp`、`TransitionPolicyTest.cpp`、`TurnCueGeometryTest.cpp`、`NavigationCueModelTest.cpp`、`Rgb565PlaneTest.cpp`，以及 SDL2 原生地图 Polygon/Polyline/Canvas、14 个页面/导航状态圆屏截图及像素有效性检查。箭头朝向测试在 CI 中曾识别并拦截一次左右翼反向错误，已修复。

### 仍未完成（不能称为全量 LVGL）

- 旧 Canvas **仍负责**复杂地图图元、栅格索引解码、车道、交通灯、部分转向图形/边缘环、天气详情和部分音乐 UI；只是绘制目标部分已接入 LVGL 的地图 Canvas 源。
- 真机原生箭头/进度环及导航中的原生滑动**默认关闭**；需安全验证所有转向、路口临近、0°/90°/314° 屏幕方向。
- `Arduino_ESP32RGBPanel` / ST7701S 初始化、RGB 时序、VSYNC、DMA、背光与 480×480 实际像素输出仍须保留板级适配器；LVGL 不取代驱动。
- 统一 framebuffer 所有权已建立第一版，但仍有一份额外的地图 PSRAM 源与 RGB 帧快照，**未完成零拷贝/最终去除 Arduino Canvas**；需要实体板子测 FPS、PSRAM 峰值、上传大包时任务并发、重启回退等。

**验收顺序**：先拿模拟器用各种导航状态核对箭头、道路和地图层，再进行硬件刷写的独立评审；检查车道/摄像头/交通灯叠层、异常角度、帧率以及长时间 BLE 断连；确认通过后才启用原生导航提示，并考虑删掉剩余 Canvas UI。

## 验证标准

- 桌面 CI：LVGL SDL2 编译、六个页面渲染、RasterBridge 纯算法测试。
- ESP32 CI：`python3 firmware/build.py --board 21 --jobs 2` **只编译，不烧录**。
- 实机：天气主/详情来回切、图 7–11 分片到达/空图清除/断连、封面和歌词、任意角度旋转/滑动、内存稳定性。
- 不声称 LVGL 像素与 Canvas 完全一致；特别留意位图 Alpha 混合和 Canvas 回退重绘在页面切换后是否残留。
