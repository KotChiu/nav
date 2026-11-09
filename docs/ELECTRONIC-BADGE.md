# MotoNav 电子吧唧（Photo Badge）· 开发预览

> 独立开发分支：`feature/e-badge-photo-upload`；基于 LVGL 迁移分支，不是已发布版本。本功能当前仅针对 **Waveshare ESP32-S3-Touch-LCD-2.1**（480×480）。不建议刷入其它尺寸圆屏。

## 你可以做什么

在 iOS App 的「圆屏显示 → 电子吧唧」或 Android App 的「圆屏显示 → 电子吧唧 · 上传照片」中选择一张照片。手机本地裁剪为正方形并压缩到 **240×240 RGB565**（不是云端上传），通过已认证 BLE 连接传给设备。设备校验 CRC，保存到屏幕缓冲，并显示在独立的 Badge 页面。

停留在 Badge 页面时，照片会覆盖圆屏展示；左右滑动可以回到导航、天气、音乐等页面。**不在导航、来电或配对过程中强制抢占画面**。如果设备 FAT 文件系统可用，照片会保存在 `/badge.bin`，下一次启动自动显示；即使手机不连接，也可展示已保存图片。

注意 240×240 放大到 466×466 会损失锐度。后续可以考虑图片质量、渐进式压缩、缩放裁剪 UI 和多照片管理。

## 传输协议

沿用 MotoNav 的 `M` BLE Bulk 分片格式（type `6`，和地图 1~4、音乐封面 5 互不覆盖）。大包的完整载荷为：

| 字节 | 内容 |
| --- | --- |
| 0..3 | ASCII `BDG1` |
| 4..5 | `240`（uint16 little-endian） |
| 6..7 | `240`（uint16 little-endian） |
| 8..11 | 后续像素数据的 IEEE CRC32（uint32 little-endian） |
| 12..115211 | 240×240 RGB565，**每像素 little-endian** |

总计 **115212 字节（约 112.5 KiB）**。设备只有完成分片并通过长度、magic、宽高和 CRC 才返回 `K,6,<id>,<saved>`（尾部 saved=1/0，兼容只读取前三字段的旧客户端）。重复包确认也携带相同保存结果。iOS/Android **只接受与当前上传 ID 一致的确认**，蓝牙队列失败或 120 秒无响应会提示重试。丢片、CRC 错或写入界限错误不得 ACK。

设备在已认证连接建立时发送 `CAP,BADGE,1`。App 只在收到能力通知后开放上传。设备显示成功后额外发送：
- `BDG,SAVED,<id>,1`：FAT 文件持久保存成功；
- `BDG,SAVED,<id>,0`：RAM 显示成功，但 FAT 未能挂载/写入，**下次断电可能丢失**。\n\n确认包含上传 ID，避免上一张照片的迟到通知误报下一张照片的状态。

## 存储安全

开机时优先挂载 2.1 圆屏上已插入的 FAT32 microSD，吧唧照片保存到 `/motonav/badge.bin`。已有的内部 FAT 照片仍可读取，没有可挂载的 SD 时可以继续使用内部 FAT。两种存储都无法挂载时，照片可在本次开机期间显示，手机会收到未持久保存的状态。固件不会自动格式化任何存储。插卡后重启圆屏即可重新检测。

替换时先完整写入并检查临时文件长度，再把旧图保留在 `.bak`，最后提交新图；如果断电或新图损坏，启动时会尝试从上一个已校验的 `.bak` 恢复。**这不是数据库级事务**，仍可能受到 FAT 分区损坏、介质故障影响；手机原图应保留。所有图片均从本地手机选择，经 BLE 发给已验证的设备，不经过自建云服务。

## 构建与测试

```bash
# 不连接开发板，只构建固件
python3 firmware/build.py --board 21

# 不烧录的 Mac/PC LVGL 模拟
cmake -S tools/lvgl-simulator -B build/lvgl-simulator
cmake --build build/lvgl-simulator
./build/lvgl-simulator/motonav-lvgl-simulator

# 用电脑上的真实照片预览：本地裁剪编码，不上传网络，也不烧录\npython3 -m pip install Pillow\npython3 tools/lvgl-simulator/make_badge.py ~/Pictures/photo.jpg /tmp/motonav-photo.bdg\n./build/lvgl-simulator/motonav-lvgl-simulator --badge-file /tmp/motonav-photo.bdg\n\n# 纯逻辑协议测试
g++ -std=c++17 firmware/tests/BadgePacketTest.cpp -o /tmp/badge-test && /tmp/badge-test
```

Android：`cd android && ./gradlew :app:assembleDebug`；iOS 使用 Xcode workspace 与原有 CocoaPods 锁定依赖。GitHub Actions 分别执行 Android/iOS、固件、LVGL 的构建检查。

## 真机准出（未执行前不要标记完成）

- [ ] iOS 和 Android 选择不同方向、尺寸和透明图片，预览方向与实机一致
- [ ] 设备认证后出现 `CAP,BADGE,1`，旧固件不显示错误的上传入口
- [ ] 新图上传后收到 `K,6,id`；CRC 错/缺片/断连不能假成功
- [ ] 停车/非导航时上传成功自动进入 Badge；导航/配对/来电不被图片遮挡
- [ ] 手势左右切页，包含从 Settings 进入 Badge，退出后正常恢复天气/音乐/导航
- [ ] 设备重启离线展示、FAT 无法挂载提示、异常断电与存储已满处理
- [ ] 画面圆形边界、0°/90°/314° 旋转、触摸方向与 PSRAM/CPU 占用
- [ ] 上传期间模拟地图更新，导航控制命令不丢失或长时间阻塞

## 后续可迭代

支持图片拖拽裁剪/缩放、多个吧唧图片、图片切换、GIF 动态吧唧、高清压缩（提高到 400+ px）、照片自动轮播和桌面模拟器加载本地图，而不是仅使用彩色演示图。
