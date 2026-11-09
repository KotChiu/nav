# MotoNav 2.1 · LVGL 桌面模拟器（实验）

本目录编译 **LVGL 8.3.10 + SDL2** 桌面程序，窗口 480×480，中央按设备现行逻辑画布 466×466 显示圆形区域。**无需 ESP32、Arduino CLI、烧录或手机**。

## 本轮已经复用的真实固件代码

- `firmware/boards/21/MotoNav/ui/LvglHomeChrome.h`：主页四段青色环和品红顶部标记，已接入 2.1 固件 `LvglOverlay.h`，桌面模拟器使用**同一文件**绘制。
- `firmware/boards/21/MotoNav/ui/LvglWeatherView.h`：天气主页面的温度、天气状态、未来时段与空气质量状态，真机读取 BLE 解码结果，模拟器注入固定 fixture。天气详情/中文文字位图暂时走设备原 Canvas。
- `firmware/boards/21/MotoNav/ui/LvglMusicStatusView.h`：音乐页标题、播放状态和操作提示，真机使用 HID/AMS/音乐反馈状态；模拟器使用可点击的模拟播放状态。
- 2.1 固件 `HomePage.h` 在 LVGL 初始化后不再用 `Arduino Canvas` 重复绘制主页装饰；启动早期仍保留传统 Canvas 回退，避免黑屏。

## 本轮的模拟数据（不是固件截图/真机等价预览）

**天气主页面和音乐播放状态使用了设备/桌面共享的 LVGL 控件实现，但桌面仍注入固定模拟数据。** **电子吧唧第七页也使用了设备端共用的 `LvglBadgeView.h` 图片控件，但显示的是程序生成的彩色测试图，不是从手机接收的实际照片。** 导航路线、股票、互动角色、设置仍以桌面示例控件为主。原设备的天气详情、中文位图、音乐封面/歌词、导航地图和页面滑动仍需继续迁移。模拟器不是 ESP32 的像素级回归结果。

键盘：← → 换页、数字无；N 开关模拟导航、B 切换模拟手机连接、↑ ↓ 调整模拟车速、Esc 退出。鼠标点击按钮、左右拖动切页、下滑回主页、滚轮切页；设置页滑杆为模拟值，不会操作硬件。

## macOS

```bash
brew install cmake sdl2
cmake -S tools/lvgl-simulator -B build/lvgl-simulator -DCMAKE_BUILD_TYPE=Release
cmake --build build/lvgl-simulator -j4
./build/lvgl-simulator/motonav-lvgl-simulator
```

CMake 配置时使用 Git 获取指定标签的 LVGL v8.3.10（首次需要联网）。macOS 上使用 Homebrew 安装的 SDL2；缺少时可能需要向 CMake 提供 `SDL2_DIR`。

## Ubuntu / Debian

```bash
sudo apt-get install -y cmake git g++ libsdl2-dev
cmake -S tools/lvgl-simulator -B build/lvgl-simulator -DCMAKE_BUILD_TYPE=Release
cmake --build build/lvgl-simulator -j4
./build/lvgl-simulator/motonav-lvgl-simulator
```

无桌面服务器的 CI 可用 `xvfb-run -a ctest --test-dir build/lvgl-simulator --output-on-failure` 跑快速启动冒烟检查。

## 架构与迁移边界

LVGL 是 UI 框架，不是 ST7701S RGB DMA 驱动。设备端必须保留一层 `esp_lcd` / ESP32 RGB / Arduino_RGB_Display 这样的硬件显示适配器，并由 LVGL 的 flush 接口或共享 frame buffer 接到该层。**本轮不移除设备底层 RGB 驱动**，因为其负责面板初始化、时序、帧缓冲和 VSYNC。

完整迁移需要分阶段完成：
1. 【已完成阶段 1】主页环 LVGL 原生化、共享组件、PC SDL2 模拟器雏形；
2. 【阶段 2 部分完成】天气摘要、音乐状态共享 LVGL UI，真机接真实状态；天气详情、音乐封面和中文位图尚待转换为 LVGL image/canvas；
3. 【后续】导航、页面手势、图像/文字位图独立成可注入数据的 LVGL widgets，设备与 PC 复用同一份 UI；
4. 【后续】替换 `gfx->fill*`、`getFramebuffer()`、自定义滑动合成与地图 Canvas 绘制，定义帧缓冲所有权；
5. 【后续】将 LVGL 的刷新接口接入板级 RGB 硬件适配层，做 0°/90°/314°、PSRAM、掉线与触摸实机回归；
6. 【最后】在实际设备构建和画面一致性通过后，再考虑删除 Arduino Canvas/Arduino_GFX 依赖。部分必须逐像素绘制的地图仍可使用 LVGL canvas / image 对象，不一定改成标签/按钮。

请把桌面编译通过、模拟器可用、ESP32 构建通过、实机画面一致及道路测试视为**不同验收项**。

> 编译原 2.1 固件依旧使用 `python3 firmware/build.py --board 21`。不要把 LVGL 模拟器编译当成固件编译。
