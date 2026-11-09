# AGENTS.md — MotoNav AI Coding 开发约定

> 适用于本仓库根目录及所有子目录。目标是让 Codex 等 AI Coding 工具先识别板型、协议与验证边界，再做最小且可回归的修改。若子目录有更具体的 AGENTS.md，以其额外约束为准。

## 1. 项目和修改范围

MotoNav = Android / iOS 手机 App + BLE + ESP32-S3 圆屏固件。**手机负责导航算路、联网与地图/天气/行情等数据采集；固件负责本地屏幕、触摸、设备状态、显示和 BLE。** 不要把 ESP32 当作独立的联网导航设备。

- 当前主要开发分支：`feature/board-2.1`。
- 2.1 固件：`firmware/boards/21/MotoNav/`，Waveshare ESP32-S3-Touch-LCD-2.1，**480×480 ST7701S RGB + CST820**。当前逻辑画布仍为 **466×466**，由板级输出缩放/旋转到 480×480。
- 1.75 固件：`firmware/boards/175/MotoNav/`，Waveshare ESP32-S3-Touch-AMOLED-1.75，466×466 AMOLED。和 2.1 的面板驱动/初始化/烧录镜像不同。
- 1.85B 固件：`firmware/boards/185b/`，360×360 实验版；不能把它当作最新的 2.1 / 1.75。
- Android：`android/`，原版 8.3.26；iOS：`ios/`，当前源码版本以 `RELEASE_VERSION` 为准。
- Android 红绿灯实验单独在 `experiments/traffic-lights/`；**不得默认并入普通版，更不得编造倒计时数据**。

**任何固件、分区、管脚、RGB 时序、触摸驱动、NVS 和 OTA/烧录变更，先确定目标板型；禁止跨板混刷。**

## 2. 开始任务前

1. 确认当前分支、需求适用端（Android / iOS / 175 / 21 / 185b）和改动边界。
2. 先读 `README.md`、`docs/ARCHITECTURE.md`、`docs/ROADMAP.md`；2.1 UI/性能任务额外读 `docs/BOARD-2.1-RESEARCH.md`、`docs/HANDOFF-2026-11-08-esp32-black-theme.md`、`docs/UI-PERFORMANCE-2026-11-08.md`。
3. 同时查调用方和数据生产方；跨 BLE 协议改动要查 **两端 App + 各受影响板型固件**，保持旧版兼容或明确版本协商策略。
4. 优先小变更和可运行的回归测试，不为美观任意大规模搬动单文件编译单元或全局状态。

## 3. 固件代码约束

- 2.1 程序入口是 `firmware/boards/21/MotoNav/MotoNav.ino`，其 `src/**` 头文件按顺序包含、当前在同一编译单元中编译；抽成独立 .cpp 前必须处理前置声明、全局对象生命周期和链接关系。
- 2.1 显示关键路径：`src/display/DisplayPipeline.h`（双缓冲/旋转/刷屏）、`src/display/LvglOverlay.h`（LVGL 8.3）、`src/pages/`（页面）、`Board21Panel.h`（RGB/触摸硬件）。**不要把所有旋转路径当作同一性能问题**：0° 有逐行复制和局部区域路径，任意角度采用最近邻逆变换；优化应按角度分组测量。
- 当前 UI 是近黑底、白色正文、青色主强调、品红辅助强调；天气、来电、股票涨跌保留语义色。不要无需求改成全灰阶；不得误改 LCD 颜色极性。
- 保护 BLE 安全握手、加密认证、消息边界、分片 ACK 和重传语义；禁止在 BLE 回调中引入耗时绘制或阻塞等待。
- 避免大块循环内动态分配，检查 PSRAM 分配与任务/信号量创建失败；更新共享缓冲时要考虑跨核时序与源缓冲生命周期。
- 超时应有明确错误处理/降级路径，不能把“等 3 秒之后永久等待”当作故障恢复。
- 不要根据理论速度宣称 FPS。记录输入到可见画面的端到端延迟、p50/p95、转角、PSRAM/堆占用、丢帧与撕裂。

## 4. 手机端与外部服务

- 地图以现有高德实现为准；天气以现有和风天气实现为准。网页摩托车路线及读秒实验都可能受第三方接口、账号、地区与网络限制。
- 手机负责股票行情/位图渲染及网络访问，固件不保存凭据。异步请求在页面/周期/设备状态改变时，应校验版本或代际 ID，防止过期回调写入新状态。
- 不提交地图/天气 Key、私有设备信息、签名证书、Cookie、定位回放、私有开机图片、调试日志或 `android/keystore`、`android/personal.properties`。

## 5. 构建、测试与验证

在仓库根目录执行（运行前确认工具和 SDK 已安装）：

```bash
# 2.1 / 1.75 固件：仅编译
python3 firmware/build.py --board 21
python3 firmware/build.py --board 175

# 2.1 macOS 烧录前只检查（不写芯片）
python3 firmware/flash_macos.py --board 21 --check

# 固件现有纯逻辑测试（需 g++ / cc）
for test in firmware/tests/*Test.cpp; do g++ -std=c++17 "$test" -o /tmp/motonav-test && /tmp/motonav-test || exit 1; done
cc firmware/tests/idle-power-off-test.c -o /tmp/motonav-idle && /tmp/motonav-idle
python3 -m unittest discover -s firmware/tests -p 'test_flash_macos.py'

# iOS 仅限具备 Xcode 的 macOS；Android 根据环境运行 Gradle 与 android/tests 脚本
bash scripts/build-ios-local.sh
```

`firmware/build.py` **只编译，不烧录**。不要在未得到明确指令、核对设备和分区之前运行 `--full` 或对真实硬件烧写。禁止把编译通过、模拟数据、串口初始化和真实道路验证混为一谈。若无法运行构建/真机测试，明确写“未运行”及原因。

## 6. 交付规范

每次改动说明：**需求与板型 → 修改文件 → 兼容性影响 → 已运行测试及输出 → 未验证范围与风险 → 回滚办法**。修复性能问题优先提供基线和优化后同场景指标。修改公开版本/Release 信息时，区分源码当前版本与 GitHub Releases 已发布成品；变更协议时同步 `docs/ARCHITECTURE.md`、相应测试和发布说明。

参考：[架构](docs/ARCHITECTURE.md) · [路线图](docs/ROADMAP.md) · [代码优化审查](docs/CODE-REVIEW-2026-11-09.md) · [项目许可](LICENSE.md)。
