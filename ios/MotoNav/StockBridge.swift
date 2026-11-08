import Foundation
import Combine

/// Fetches a small market watchlist on the iPhone and forwards compact quotes
/// to the display. Keeping network credentials and parsing on the phone avoids
/// putting third-party API details in firmware.
final class StockBridge: ObservableObject {
    static let shared = StockBridge()

    struct Quote: Identifiable {
        var id: String { code }
        let code: String
        let price: Double
        let changePct: Double
    }

    @Published private(set) var watchlist: [String]
    @Published private(set) var quotes: [Quote] = []
    @Published private(set) var status = "等待刷新"
    @Published private(set) var lastUpdated: Date?

    private let defaultsKey = "stock_watchlist_v1"
    private var timer: Timer?
    private var fetching = false

    private init() {
        let saved = UserDefaults.standard.stringArray(forKey: defaultsKey) ?? []
        watchlist = saved.isEmpty ? ["sh000001", "sz399001", "usAAPL"] : Array(saved.prefix(4))
    }

    func start() {
        guard timer == nil else { return }
        refresh()
        timer = Timer.scheduledTimer(withTimeInterval: 60, repeats: true) { [weak self] _ in
            guard let self = self, BleLink.shared.isReady else { return }
            self.refresh()
        }
    }

    func refresh() {
        guard !fetching, !watchlist.isEmpty else { return }
        fetching = true
        status = "正在更新行情…"
        let codes = watchlist
        guard let url = URL(string: "https://qt.gtimg.cn/q=\(codes.joined(separator: ","))") else {
            fetching = false
            status = "股票代码格式不正确"
            return
        }
        var request = URLRequest(url: url, timeoutInterval: 12)
        request.setValue("Mozilla/5.0 (iPhone; CPU iPhone OS 17_0 like Mac OS X)", forHTTPHeaderField: "User-Agent")
        request.setValue("https://gu.qq.com/", forHTTPHeaderField: "Referer")
        URLSession.shared.dataTask(with: request) { [weak self] data, response, error in
            guard let self = self else { return }
            let parsed: Result<[Quote], Error>
            if let error = error {
                parsed = .failure(error)
            } else if let http = response as? HTTPURLResponse, !(200..<300).contains(http.statusCode) {
                parsed = .failure(StockError.http(http.statusCode))
            } else if let data = data,
                      let text = String(data: data, encoding: .isoLatin1) {
                parsed = .success(Self.parse(text, requested: codes))
            } else {
                parsed = .failure(StockError.invalidData)
            }
            DispatchQueue.main.async {
                self.fetching = false
                switch parsed {
                case .success(let values):
                    if values.isEmpty {
                        self.status = "没有收到行情，检查股票代码"
                    } else {
                        self.quotes = values
                        self.lastUpdated = Date()
                        self.status = "已更新 \(values.count) 只 · \(Self.timeText(Date()))"
                        self.sendLatest()
                    }
                case .failure(let error):
                    self.status = "行情更新失败：\(error.localizedDescription)"
                }
            }
        }.resume()
    }

    @discardableResult
    func add(_ raw: String) -> Bool {
        guard watchlist.count < 4, let code = Self.normalize(raw) else { return false }
        guard !watchlist.contains(code) else { return false }
        watchlist.append(code)
        save()
        quotes.removeAll()
        refresh()
        return true
    }

    func remove(_ code: String) {
        watchlist.removeAll { $0 == code }
        quotes.removeAll { $0.code == code }
        save()
        sendLatest()
        refresh()
    }

    /// Re-send the last successful payload after authentication or reconnect.
    func sendLatest() {
        guard BleLink.shared.isReady, !quotes.isEmpty else { return }
        var parts = ["#STK", String(min(quotes.count, 4))]
        for q in quotes.prefix(4) {
            parts.append(q.code.uppercased())
            parts.append(String(format: "%.2f", q.price))
            parts.append(String(format: "%+.2f", q.changePct))
        }
        BleLink.shared.send(parts.joined(separator: ","))
    }

    private func save() {
        UserDefaults.standard.set(watchlist, forKey: defaultsKey)
    }

    private static func normalize(_ raw: String) -> String? {
        let value = raw.trimmingCharacters(in: .whitespacesAndNewlines).uppercased()
        guard !value.isEmpty, value.count <= 18 else { return nil }
        let prefixes = ["SH", "SZ", "BJ", "HK", "US"]
        for prefix in prefixes where value.hasPrefix(prefix) && value.count > 2 {
            return prefix.lowercased() + value.dropFirst(2)
        }
        if value.allSatisfy({ $0.isNumber }) {
            if value.count == 5 { return "hk" + value }
            if value.first == "6" || value.first == "5" || value.first == "9" {
                return "sh" + value
            }
            return "sz" + value
        }
        let allowed = CharacterSet.alphanumerics.union(CharacterSet(charactersIn: ".-"))
        guard value.unicodeScalars.allSatisfy({ allowed.contains($0) }) else { return nil }
        return "us" + value
    }

    private static func parse(_ text: String, requested: [String]) -> [Quote] {
        let wanted = Set(requested.map { $0.lowercased() })
        var found: [String: Quote] = [:]
        for line in text.components(separatedBy: ";") {
            guard let start = line.range(of: "=\""),
                  let keyStart = line.range(of: "v_") else { continue }
            let key = line[keyStart.upperBound..<start.lowerBound]
                .trimmingCharacters(in: .whitespacesAndNewlines)
                .lowercased()
            guard wanted.contains(key) else { continue }
            var body = String(line[start.upperBound...])
            if body.hasSuffix("\"") { body.removeLast() }
            let fields = body.split(separator: "~", omittingEmptySubsequences: false).map(String.init)
            guard fields.count > 4,
                  let price = Double(fields[3]),
                  price > 0 else { continue }
            let previous = Double(fields[4]) ?? 0
            let reported = fields.count > 32 ? Double(fields[32]) : nil
            let pct = reported ?? (previous > 0 ? (price - previous) / previous * 100 : 0)
            found[key] = Quote(code: key.uppercased(), price: price, changePct: pct)
        }
        return requested.compactMap { found[$0.lowercased()] }
    }

    private static func timeText(_ date: Date) -> String {
        let f = DateFormatter()
        f.locale = Locale(identifier: "zh_CN")
        f.dateFormat = "HH:mm"
        return f.string(from: date)
    }
}

private enum StockError: LocalizedError {
    case http(Int)
    case invalidData

    var errorDescription: String? {
        switch self {
        case .http(let code): return "服务器返回 \(code)"
        case .invalidData: return "行情数据格式错误"
        }
    }
}
