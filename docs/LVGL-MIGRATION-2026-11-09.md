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
| 导航地图与 HUD | **新增** `ui/LvglNavigationHud.h`：地图上的速度、限速牌、摄像头距离使用和 PC 共享的 LVGL 控件；真实值来自 ESP32 导航状态 | 地图底图、多边形、路线、红绿灯、车道、转向提示、进度环仍使用 Canvas，需要后续迁移 |
| 滑动切页 | **新增** `ui/LvglPageSlide.h`：设置/互动角色/股票之间的滑动复用原有两张 PSRAM 快照，LVGL 两张 image 移动代替逐行 memcpy | 含导航、天气、音乐的混合绘制仍走旧合成器；需要 2.1 实机测试帧率、PSRAM、触摸、旋转 |

## 本轮变更

- `LvglOverlay.h`：共享 LVGL 位图资产由 5 个扩为 10 个，覆盖手机推送的天气详情中文字图。根据 `wxDetail` 和当前页面显示相应槽位，不在其他页面泄露。
- `WeatherPage.h`：天气详情页优先展示已经准备好的 LVGL 字图，未准备好时沿用 Canvas 画法。QWeather 原始雨量柱状图、提醒文本保留。
- `TextBitmapReceiver.h`：槽位 7–11 收到新图或清除时，同步标记天气页面 dirty；拒绝分片号越界、空 payload、错误 offset/超出图片总长及空接收缓冲，避免非法位移与越界写入；保持手机 BLE 包格式不变。
- `RasterBridgeTest.cpp`：单元测试 4-bit 灰度转换、透明像素、裁剪尺寸、专辑封面圆形裁切；新增 GitHub Actions 运行命令。

## 本轮后续变更：导航 HUD 与原生页面滑动

- `LvglNavigationHud.h` 使用 LVGL label、圆形限速牌与动态颜色，接入 `LvglOverlay.h`；`MapFrame.h` 在 LVGL 已启动时不再重复绘制旧速度 HUD，未启动时保留 Canvas 回退。状态包括实时车速、超速颜色、限速及摄像头距离。
- `MapState.h` 通过 accessor 把白天/夜间地图主题色提供给 LVGL 层，避免把地图内部宏硬编码进先于地图初始化的头文件中。
- `ui/MapProjection.h` 提取地图世界坐标到 466×466 逻辑画布的旋转/缩放变换；`MapFrame.h`、`RouteRenderer.h`、`RadarEffects.h` 已复用相同计算，桌面导航示例也共用此函数。`MapProjectionTest.cpp` 验证不同方向/比例下与旧 Canvas 数学公式一致，**尚未替换 Canvas 地图填充和画线 API**。
- `LvglPageSlide.h` + `PageSlideGeometry.h` 解耦滑动几何计算与显示实现。真机设置、互动角色、股票三页的相互滑动只移动 LVGL 对象，不再逐行 memcpy；两张原有 PSRAM 快照仍复用。混合绘制页**没有**启用此路径，避免地图突然空白。
- `LvglOverlay.h` 保持地图正常的异步刷新节奏；LVGL HUD 更新不会强制把地图变成同步刷新。
- 桌面模拟器在导航预览中使用相同 `LvglNavigationHud`，并接入原生滑动层；数据及道路仍是模拟的。
- `PageSlideGeometryTest.cpp` 验证正反方向与横竖滑动全过程无缝覆盖；SDL2 冒烟检查创建导航 HUD 和原生滑动层。

**仍未完成**：统一的 LVGL 地图图元/底图、路线与转向覆盖，触摸到刷新端到端性能与安全验收，彻底去除 Canvas，物理 RGB DMA/PSRAM 的专有适配层。当前为功能迁移的一部分，不可直接宣称全部 LVGL 迁移完成。

## 验证标准

- 桌面 CI：LVGL SDL2 编译、六个页面渲染、RasterBridge 纯算法测试。
- ESP32 CI：`python3 firmware/build.py --board 21 --jobs 2` **只编译，不烧录**。
- 实机：天气主/详情来回切、图 7–11 分片到达/空图清除/断连、封面和歌词、任意角度旋转/滑动、内存稳定性。
- 不声称 LVGL 像素与 Canvas 完全一致；特别留意位图 Alpha 混合和 Canvas 回退重绘在页面切换后是否残留。
