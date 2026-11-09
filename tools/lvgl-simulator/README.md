# MotoNav 2.1 · LVGL 桌面模拟器（实验）

本目录编译 **LVGL 8.3.10 + SDL2** 桌面程序，窗口 480×480，中央按设备现行逻辑画布 466×466 显示圆形区域。**无需 ESP32、Arduino CLI、烧录或手机**。

## 本轮已经复用的真实固件代码

- `firmware/boards/21/MotoNav/ui/LvglHomeChrome.h`：主页四段青色环和品红顶部标记，已接入 2.1 固件 `LvglOverlay.h`，桌面模拟器使用**同一文件**绘制。
- 2.1 固件 `HomePage.h` 在 LVGL 初始化后不再用 `Arduino Canvas` 重复绘制主页装饰；启动早期仍保留传统 Canvas 回退，避免黑屏。

## 本轮的模拟数据（不是固件截图/真机等价预览）

天气、导航路线、音乐、股票、互动角色、设置目前使用 **桌面 LVGL 示例组件与固定模拟数据**，仅用于交互方向、排版与共享组件提取。**还没有移植原固件各页面的全部真实绘制代码**，不得把这个模拟器视为 ESP32 的像素级回归结果。

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
1. 【本轮】主页环 LVGL 原生化、共享组件、PC SDL2 模拟器雏形；
2. 【后续】把天气、音乐、导航、页面手势、图像/文字位图独立成可注入数据的 LVGL widgets，设备与 PC 复用同一份 UI；
3. 【后续】替换 `gfx->fill*`、`getFramebuffer()`、自定义滑动合成与地图 Canvas 绘制，定义帧缓冲所有权；
4. 【后续】将 LVGL 的刷新接口接入板级 RGB 硬件适配层，做 0°/90°/314°、PSRAM、掉线与触摸实机回归；
5. 【最后】在实际设备构建和画面一致性通过后，再考虑删除 Arduino Canvas/Arduino_GFX 依赖。部分必须逐像素绘制的地图仍可使用 LVGL canvas / image 对象，不一定改成标签/按钮。

请把桌面编译通过、模拟器可用、ESP32 构建通过、实机画面一致及道路测试视为**不同验收项**。

> 编译原 2.1 固件依旧使用 `python3 firmware/build.py --board 21`。不要把 LVGL 模拟器编译当成固件编译。
