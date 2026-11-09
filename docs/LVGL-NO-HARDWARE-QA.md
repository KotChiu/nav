# LVGL 迁移：无真机阶段的代码与质量验收

> 状态：Draft PR #2；尚未合并、未烧录、未验证物理 2.1 英寸显示板。此阶段不要求用户持有开发板。

## 哪些部分能先完成

- ESP32-S3 固件必须能够针对真实的 2.1 英寸 Waveshare 屏幕**编译通过**，但不能据此宣称能够点亮屏幕或稳定导航。
- 当前主绘制表面已提供 **不继承 `Arduino_Canvas`** 的 `MotoNavRasterSurface`；它继承 `Arduino_GFX` 以兼容旧图形算法，属于迁移过渡层，并非纯 LVGL 画法。CI 分别编译这个新路径和原来的 `Arduino_Canvas` 回退路径。
- Mac/Linux 上的 LVGL 8.3 SDL2 模拟器必须能够运行所有 6 个当前固件页面，复用真实天气/音乐/导航 HUD、地图 Canvas、多边形及路线绘制模块，**数据来源是固定场景，不是 BLE/GPS**。
- 纯 C++ 单元测试应覆盖：地图坐标投影、手势翻页/几何覆盖、导航箭头方位、所有方向类型、图像像素/解码和 framebuffer 不重叠等。
- 每次 PR 提交可运行 CI，无设备也会生成 14 张 480×480 圆屏场景 PNG，以及一个可查看的 `index.html`。来自模拟器，**不是物理屏幕截图**。
- 风险敏感的旧路径不能为了“代码收尾”而提前删除：新地图缺 PSRAM 或出现复杂路口时可回退，原生箭头和导航中翻页有编译开关防护。

## 在 Mac 上直接跑

```bash
brew install cmake sdl2
cmake -S tools/lvgl-simulator -B build/lvgl-simulator -DCMAKE_BUILD_TYPE=Release
cmake --build build/lvgl-simulator -j4

# 单独编译 2.1 新旧绘制路径（不会烧录）
python3 firmware/build.py --board 21 --raster-surface native
python3 firmware/build.py --board 21 --raster-surface legacy

# 普通交互模式：左右键换页，N 开关导航
./build/lvgl-simulator/motonav-lvgl-simulator

# 离线批量测试：生成 14 份模拟圆屏图
./build/lvgl-simulator/motonav-lvgl-simulator --smoke-test --capture-dir build/motonav-visual
python3 tools/lvgl-simulator/verify_captures.py build/motonav-visual
open build/motonav-visual/index.html
```

Linux CI 使用 `xvfb-run` 启动无界面 SDL2，并在 `LVGL desktop simulator` 工作流的 **motonav-lvgl-visual-gallery** Artifact 内附 PNG 和 HTML。

场景清单：主页/天气/音乐/互动角色/股票/设置、直行、左转、右转、接近路口、超速、掉头、到达终点、手机离线主页。

自动图像检查确保：所有图片均为 480×480 圆屏、四角和边界遮罩为黑色、内部不是近乎空白；关键左右转/进度/超速/掉头/到达/连接状态与普通场景像素存在差异。这是**基础回归**，仍需要人工看 PNG 是否符合设计预期。

## 不能在模拟器里证明的事

- Waveshare ST7701S 480×480 RGB、DMA/VSYNC、背光、电源与触摸硬件
- 屏幕朝向 0°、90°、314° 的实体转动与触摸方向同步
- BLE 大包地图/天气/电子吧唧上传造成的带宽争抢、掉帧
- 真实 PSRAM 内存压力、任务调度竞争、异常断电与 24 小时连续运行
- 路口交通灯、车道、转向环、GPS 动态信息与旧 Canvas 实机的**像素级**比较
- 左右转/U 型转弯/到达等导航关键元素是否有足够大、足够清晰及安全的显示效果

## 当前策略与明确的完成标准

- **软件构建通过**：ESP32 编译 + LVGL 模拟器 + 14 图图像测试 + 纯逻辑测试全绿。
- **界面迁移代码就绪**：可以运行共享的 LVGL Canvas/line/polygon/map image、导航 HUD、基本转向及 6 页面翻页，保留旧特殊图元和解码兼容路径。不要误称“完全无 Canvas”。
- **真机验收完成**（尚未达到）：有实物后验证所有导航场景、PSRAM 与 FPS、旋转、来电与导航的优先级，才允许打开 `MOTONAV_NATIVE_NAV_CUES` 和 `MOTONAV_NATIVE_NAV_SLIDES`，逐步删除最后的 Arduino Canvas UI 代码。
- **正式可发布**（尚未达到）：真机连续运行回归成功，且通过图片、路线和紧急提示正确性检查；再合并 Draft PR。

在开发者没有实体硬件的时期，**不自动开真机独有开关，不合并到现有导航固件、不把 CI 当成实机通过**。
