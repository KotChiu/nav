import Foundation
import Compression
import UIKit
import MediaPlayer
import Combine

/// 专辑封面推送到圆屏（bulk type 5）。
/// 来源优先级：App 模式直读 nowPlayingItem.artwork → HID 模式本地曲库匹配（标题+歌手）→ iTunes 搜索 API 兜底。
/// 编码：居中裁方 → 128×128 RGB565（小端）→ zlib，头 'C''V''R' 版本 宽 高。
final class CoverBridge {
    static let shared = CoverBridge()
    static let px = 128
    private let cache = NSCache<NSString, UIImage>()
    private var lastKey = ""
    private var seq = 0
    private var fetching = false

    /// 每次歌曲文本变化时调用；title 为空表示没有播放会话，清掉封面。
    func update(title: String, artist: String, item: MPMediaItem?) {
        guard !title.isEmpty else {
            if lastKey != "clear" { lastKey = "clear"; send(header(w: 0, h: 0, data: Data())) }
            return
        }
        let key = title + "|" + artist
        guard key != lastKey, !fetching else { return }
        lastKey = key
        if let img = cachedOrLocal(key: key, item: item, title: title, artist: artist) {
            push(img)
            return
        }
        fetching = true
        fetchArtwork(title: title, artist: artist) { [weak self] img in
            DispatchQueue.main.async {
                guard let self = self else { return }
                self.fetching = false
                if let img = img {
                    self.cache.setObject(img, forKey: key as NSString)
                    self.push(img)
                } else {
                    self.send(self.header(w: 0, h: 0, data: Data()))      // 没找到：清掉上一首的封面
                }
            }
        }
    }

    private func cachedOrLocal(key: String, item: MPMediaItem?, title: String, artist: String) -> UIImage? {
        if let img = cache.object(forKey: key as NSString) { return img }
        if let art = item?.artwork?.image(at: CGSize(width: 256, height: 256)) { return art }
        return libraryArtwork(title: title, artist: artist)
    }

    /// 本地曲库匹配（离线；对网易云/QQ 音乐的歌只在同时存在于资料库时有封面）
    private func libraryArtwork(title: String, artist: String) -> UIImage? {
        guard MPMediaLibrary.authorizationStatus() == .authorized, !title.isEmpty else { return nil }
        let size = CGSize(width: 256, height: 256)
        let byTitle = MPMediaQuery()
        byTitle.addFilterPredicate(MPMediaPropertyPredicate(value: title, forProperty: MPMediaItemPropertyTitle))
        if !artist.isEmpty {
            let both = MPMediaQuery()
            both.addFilterPredicate(MPMediaPropertyPredicate(value: title, forProperty: MPMediaItemPropertyTitle))
            both.addFilterPredicate(MPMediaPropertyPredicate(value: artist, forProperty: MPMediaItemPropertyArtist))
            if let art = both.items?.first?.artwork?.image(at: size) { return art }
        }
        return byTitle.items?.first?.artwork?.image(at: size)
    }

    /// iTunes 搜索兜底（联网）。artworkUrl100 把 100x100bb 换成 600x600bb 拿高清图。
    private func fetchArtwork(title: String, artist: String, done: @escaping (UIImage?) -> Void) {
        var comps = URLComponents(string: "https://itunes.apple.com/search")!
        var term = title
        if !artist.isEmpty { term += " " + artist }
        comps.queryItems = [URLQueryItem(name: "term", value: term),
                            URLQueryItem(name: "entity", value: "song"),
                            URLQueryItem(name: "limit", value: "5")]
        guard let url = comps.url else { done(nil); return }
        download(url) { data in
            guard let data = data,
                  let obj = (try? JSONSerialization.jsonObject(with: data)) as? [String: Any],
                  let results = obj["results"] as? [[String: Any]] else { done(nil); return }
            let lowTitle = title.lowercased()
            let hit = results.first { r in
                let t = (r["trackName"] as? String ?? "").lowercased()
                let a = (r["artistName"] as? String ?? "").lowercased()
                return t.contains(lowTitle) || lowTitle.contains(t) || a.contains(artist.lowercased())
            } ?? results.first
            guard let urlStr = hit?["artworkUrl100"] as? String,
                  let big = URL(string: urlStr.replacingOccurrences(of: "100x100bb", with: "600x600bb")) else { done(nil); return }
            self.download(big) { imgData in done(imgData.flatMap(UIImage.init(data:))) }
        }
    }

    private func download(_ url: URL, done: @escaping (Data?) -> Void) {
        var req = URLRequest(url: url)
        req.timeoutInterval = 10
        URLSession.shared.dataTask(with: req) { data, resp, _ in
            let ok = (resp as? HTTPURLResponse)?.statusCode == 200
            done(ok ? data : nil)
        }.resume()
    }

    /// 居中裁方 → 缩放到 128×128 → RGB565 → zlib → bulk type 5
    private func push(_ image: UIImage) {
        guard let rgba = Self.rgba128(image) else { send(header(w: 0, h: 0, data: Data())); return }
        var raw = [UInt8](repeating: 0, count: Self.px * Self.px * 2)
        for i in 0..<(Self.px * Self.px) {
            let r = rgba[i * 4], g = rgba[i * 4 + 1], b = rgba[i * 4 + 2]
            let v = (UInt16(r) & 0xF8) << 8 | (UInt16(g) & 0xFC) << 3 | (UInt16(b) >> 3)
            raw[i * 2] = UInt8(v & 0xFF)
            raw[i * 2 + 1] = UInt8(v >> 8)
        }
        let z = Self.zlibWrap(raw)
        send(header(w: Self.px, h: Self.px, data: z))
    }

    /// 苹果 COMPRESSION_ZLIB 输出裸 deflate（RFC1951）；固件 parseCover 按 zlib 流（RFC1950）解析，
    /// 需补 0x78 0x9C 头 + adler32 尾（与安卓 java.util.zip.Deflater 产物一致）。
    /// 压缩失败时退回"未压缩存储块"，仍是合法 zlib 流。
    private static func zlibWrap(_ raw: [UInt8]) -> Data {
        var out = Data([0x78, 0x9C])
        // 注意：compressed(using:) 定义在 NSData 上，Data 必须先桥接（Data 本身没有这个成员）
        if let deflated = try? (Data(raw) as NSData).compressed(using: .zlib) as Data {
            out.append(deflated)
        } else {
            let n = raw.count
            out.append(contentsOf: [0x01,
                UInt8(truncatingIfNeeded: n), UInt8(truncatingIfNeeded: n >> 8),
                UInt8(truncatingIfNeeded: n >> 16), UInt8(truncatingIfNeeded: n >> 24),
                UInt8(truncatingIfNeeded: ~n), UInt8(truncatingIfNeeded: ~(n >> 8)),
                UInt8(truncatingIfNeeded: ~(n >> 16)), UInt8(truncatingIfNeeded: ~(n >> 24))])
            out.append(contentsOf: raw)
        }
        var a: UInt32 = 1, b: UInt32 = 0
        for byte in raw { a = (a + UInt32(byte)) % 65521; b = (b + a) % 65521 }
        let sum = (b << 16) | a
        out.append(UInt8((sum >> 24) & 0xFF))
        out.append(UInt8((sum >> 16) & 0xFF))
        out.append(UInt8((sum >> 8) & 0xFF))
        out.append(UInt8(sum & 0xFF))
        return out
    }

    private func send(_ data: Data) {
        BleLink.shared.sendBulk(type: 5, id: seq & 0xFF, data: data)
        seq = (seq + 1) & 0xFF
    }

    private func header(w: Int, h: Int, data: Data) -> Data {
        var d = Data([UInt8(ascii: "C"), UInt8(ascii: "V"), UInt8(ascii: "R"), 1, UInt8(w), UInt8(h)])
        d.append(data)
        return d
    }

    /// 居中裁方 + 缩放到 128×128，返回 RGBA8 字节
    private static func rgba128(_ image: UIImage) -> [UInt8]? {
        let side = CGFloat(px)
        let scale = image.scale
        let s = min(image.size.width, image.size.height)
        let cropRect = CGRect(x: (image.size.width - s) / 2 * scale, y: (image.size.height - s) / 2 * scale,
                              width: s * scale, height: s * scale)
        guard let crop = image.cgImage?.cropping(to: cropRect) else { return nil }
        var out = [UInt8](repeating: 0, count: px * px * 4)
        guard let ctx = CGContext(data: &out, width: px, height: px, bitsPerComponent: 8, bytesPerRow: px * 4,
                                  space: CGColorSpaceCreateDeviceRGB(),
                                  bitmapInfo: CGImageAlphaInfo.premultipliedLast.rawValue) else { return nil }
        ctx.interpolationQuality = .medium
        ctx.draw(crop, in: CGRect(x: 0, y: 0, width: side, height: side))
        return out
    }
}


// MARK: - Electronic badge photo (firmware BLE bulk type 6)
/// A user-selected photo is crop-to-fill resized to 240x240 RGB565.
/// Header: "BDG1" + LE16 width + LE16 height + LE32 CRC32(raw pixels).
/// No compression in v1: firmware validates exact length and CRC before ACK.
final class BadgeBridge: ObservableObject {
    static let shared = BadgeBridge()
    private static let size = 240
    private var serial = 0
    private var pendingId: Int?
    @Published var selectedImage: UIImage?
    @Published var status = "选择照片后可上传到 2.1 英寸电子吧唧。"
    @Published var sending = false

    func upload() {
        guard let image = selectedImage else { status = "请先选择一张图片"; return }
        guard BleLink.shared.isReady, BleLink.shared.badgeSupported else {
            status = "需要连接支持电子吧唧的 2.1 英寸固件"; return
        }
        guard !NaviBridge.shared.navigating && !WebMotorSession.shared.running else {
            status = "请结束导航或巡航、停车后再上传图片"; return
        }
        guard !sending else { return }
        sending = true
        status = "正在裁剪图片并生成 RGB565…"
        serial = (serial + 1) & 0xFF
        let id = serial
        DispatchQueue.global(qos: .userInitiated).async { [weak self] in
            let packet = Self.encode(image)
            DispatchQueue.main.async {
                guard let self = self, self.serial == id else { return }
                guard let packet = packet else {
                    self.sending = false; self.status = "图片处理失败，请选择其他照片"; return
                }
                guard BleLink.shared.isReady && BleLink.shared.badgeSupported else {
                    self.sending = false; self.status = "圆屏连接已断开"; return
                }
                self.pendingId = id
                self.status = "BLE 传输中（约 \(packet.count / 1024) KB），等待圆屏确认…"
                guard BleLink.shared.sendBulk(type: 6, id: id, data: packet) else {
                    self.pendingId = nil
                    self.sending = false
                    self.status = "蓝牙队列尚未就绪，未开始上传"
                    return
                }
                DispatchQueue.main.asyncAfter(deadline: .now() + 120) { [weak self] in
                    guard let self = self, self.pendingId == id else { return }
                    self.pendingId = nil
                    self.sending = false
                    self.status = "等待设备确认超时，可能未上传成功，请检查后重试"
                }
            }
        }
    }

    func acknowledge(id: Int, saved: Bool?) {
        guard pendingId == id else { return } // ignore stale/reordered ACKs
        if let saved = saved {
            didSave(id: id, saved: saved)
        } else {
            status = "已收到图片，等待存储确认…"
        }
    }

    func didSave(id: Int, saved: Bool) {
        guard pendingId == id else { return } // old uploads cannot confirm a new one
        pendingId = nil
        sending = false
        status = saved
          ? "图片已上传并保存到设备，断开手机仍可显示。"
          : "图片已在设备内存显示，但未保存成功，重启后可能丢失。"
    }

    func transferFailed(id: Int) {
        guard pendingId == id else { return }
        pendingId = nil
        sending = false
        status = "蓝牙分片发送失败，请保持连接并重新上传"
    }

    func connectionLost() {
        guard sending else { return }
        pendingId = nil
        serial = (serial + 1) & 0xFF // invalidate pending off-main image encoding
        sending = false
        status = "连接已断开，上传未确认；重新连接后可重试"
    }

    private static func crc32(_ bytes: [UInt8]) -> UInt32 {
        var crc: UInt32 = 0xFFFF_FFFF
        for byte in bytes {
            crc ^= UInt32(byte)
            for _ in 0..<8 {
                crc = (crc >> 1) ^ ((crc & 1) != 0 ? 0xEDB8_8320 : 0)
            }
        }
        return ~crc
    }

    private static func encode(_ image: UIImage) -> Data? {
        let px = size
        let canvas = UIGraphicsImageRenderer(size: CGSize(width: CGFloat(px), height: CGFloat(px)))
        let square = canvas.image { _ in
            UIColor.black.setFill()
            UIRectFill(CGRect(x: 0, y: 0, width: CGFloat(px), height: CGFloat(px)))
            let imageSize = image.size
            guard imageSize.width > 0 && imageSize.height > 0 else { return }
            let scale = max(CGFloat(px) / imageSize.width, CGFloat(px) / imageSize.height)
            let w = imageSize.width * scale, h = imageSize.height * scale
            image.draw(in: CGRect(x: (CGFloat(px)-w)/2, y: (CGFloat(px)-h)/2,
                                  width: w, height: h))
        }
        guard let cg = square.cgImage else { return nil }
        var rgba = [UInt8](repeating: 0, count: px*px*4)
        let bitmapInfo = CGBitmapInfo.byteOrder32Big.rawValue |
                         CGImageAlphaInfo.premultipliedLast.rawValue
        guard let ctx = CGContext(data: &rgba, width: px, height: px,
                                  bitsPerComponent: 8, bytesPerRow: px*4,
                                  space: CGColorSpaceCreateDeviceRGB(),
                                  bitmapInfo: bitmapInfo) else { return nil }
        ctx.draw(cg, in: CGRect(x: 0, y: 0, width: px, height: px))
        var rgb = [UInt8](repeating: 0, count: px*px*2)
        for i in 0..<(px*px) {
            let r = UInt16(rgba[4*i]), g = UInt16(rgba[4*i+1]), b = UInt16(rgba[4*i+2])
            let value = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
            rgb[2*i] = UInt8(value & 0xFF)
            rgb[2*i+1] = UInt8(value >> 8)
        }
        let crc = crc32(rgb)
        var data = Data([0x42, 0x44, 0x47, 0x31,
                         UInt8(px & 255), UInt8(px >> 8),
                         UInt8(px & 255), UInt8(px >> 8),
                         UInt8(truncatingIfNeeded: crc),
                         UInt8(truncatingIfNeeded: crc >> 8),
                         UInt8(truncatingIfNeeded: crc >> 16),
                         UInt8(truncatingIfNeeded: crc >> 24)])
        data.append(contentsOf: rgb)
        return data
    }
}
