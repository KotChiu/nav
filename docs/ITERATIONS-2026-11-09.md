# MotoNav 2026-11-09 分阶段优化（待验收）

目标分支：`codex/iterative-stability-performance-data-20261109`（从 `feature/board-2.1` 派生）。**此分支不直接烧录设备，也不等于已发布固件或安装包。**

## 阶段 1：2.1 显示可靠性

`firmware/boards/21/MotoNav/src/display/DisplayPipeline.h`

- `flushWait()`：旧路径等待 3 秒后无限阻塞；新路径超时写串口告警并 fail-closed 重启，避免在不确认刷屏任务是否仍占用 PSRAM 时继续修改画布。
- `initDoubleBuffer()`：检查首帧缓冲、第二缓冲、两枚信号量与任务创建，失败时销毁已分配的辅助资源，保留原有同步显示路径。
- **局限**：若硬件 VSYNC 长期失效，重启仍可能形成重复故障；后续需测量启动阶段与异常重启后的可诊断性，不能把它当成已经完成的无缝恢复。

## 阶段 2：性能基线（无算法变更）

`DisplayPipeline.h`

- 增加可选的 `DISPLAY_PIPELINE_METRICS`（默认 `0`，不会持续打印）。
- 开启后，分窗口输出像素转换 / VSYNC / 提交，以及 LVGL 快照复制的平均和最大阶段耗时。
- **本轮未直接改动旋转算法或激进地启用局部快照**：旧性能记录只覆盖部分阶段，尚无法证明端到端瓶颈，先取得可比较基线以免引入残影或缓冲竞态。
- 固定 0°、90°、314°、同一页面和同一设备对比；记录设备的触摸到显示延迟以及 `free heap/PSRAM`、绘制异常。

## 阶段 3：手机数据与 BLE 完整性

`ios/MotoNav/StockBridge.swift`：
- 网络股票报价与 K 线各自使用请求代际 ID（generation）；切换周期、增删自选或发起新请求时使旧结果失效。
- 旧请求即使 `cancel()` 之后仍回调，也不能覆盖当前周期 / 自选列表的数据。

`firmware/boards/21/MotoNav/src/{protocol/BulkReceiver.h,map/MapState.h,connectivity/Bluetooth.h,core/DeviceContext.h,protocol/CommandReceiver.h}`：
- 拒绝长度为 0、超出包总长的 BLE 分片，避免旧实现把越界帧截断后仍计入片数。
- 增加每次传输有效新分片的累计字节数校验；分片数正确、总字节数不匹配时重置等待完整重传，不发送成功 ACK。
- 增加 BLE RX 队列溢出计数器 `rxDroppedCount`，不改既有队列丢弃策略。
- 接收主入口增加 `n<=0` 防护。

**未解决的协议边界**：仅核对 `receivedBytes == total` 不能证明区间无重叠（长度相同但偏移重叠仍可能产生空洞）。后续可增加版本化块布局验证或覆盖位图/范围合并；同时必须兼容已安装的 Android/iOS 客户端。此外，队列溢出后目前仍可能丢中间分片，尚未新增 NACK/重传协议。

## 检查与验收

- 新增 `firmware/tests/BulkReceiver2_1Test.cpp`：含乱序、重复、空包、越界、短缺数据、解码失败后的重传测试，直接包含 2.1 的 `BulkReceiver.h`。
- 新增 `.github/workflows/logic-tests.yml`：PR 自动跑固件纯逻辑测试及刷写脚本测试，运行结果以 GitHub Actions 为准。
- 本次环境**未运行完整 Arduino 固件编译、Xcode 构建或实体硬件测试**；不要将静态审查、等价逻辑的宿主机测试、GitHub Actions 与真实设备验收混为一谈。

### 后续准出

1. GitHub Actions 测试通过，`python3 firmware/build.py --board 21` 和 `--board 175` 编译通过。
2. 实物测试屏幕全页面，0°/90°/314° 旋转、滑动、音乐、导航，确认无残影/死锁/异常重启。
3. 真实手机验证 iOS 日 K/分钟 K 快速切换与中途删除，断连和重连 BLE，数据不会倒退。
4. 压测大量地图/封面分片与队列拥塞：失败不得误 ACK，导航高优先级不被持续延迟。

本次仅提供可审核的开发改动，不自动合并到现用分支。
