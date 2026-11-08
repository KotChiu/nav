import Foundation
import Combine
import CoreFoundation

/// Fetches a small market watchlist on the iPhone and forwards compact quotes
/// to the display. Keeping network credentials and parsing on the phone avoids
/// putting third-party API details in firmware.
final class StockBridge: ObservableObject {
    static let shared = StockBridge()

    enum ChartPeriod: String, CaseIterable {
        case day
        case minute

        var title: String {
            switch self {
            case .day: return "日 K"
            case .minute: return "1 分 K"
            }
        }

        var protocolCode: String {
            self == .day ? "D" : "M"
        }
    }

    struct Quote: Identifiable {
        var id: String { code }
        let code: String
        let name: String
        let price: Double
        let changePct: Double
    }

    struct Candle { let open: Double; let close: Double; let high: Double; let low: Double }
    @Published private(set) var candles: [String: [Candle]] = [:]
    @Published private(set) var period: ChartPeriod
    @Published private(set) var watchlist: [String]
    @Published private(set) var quotes: [Quote] = []
    @Published private(set) var status = "等待刷新"
    @Published private(set) var lastUpdated: Date?

    private let defaultsKey = "stock_watchlist_v1"
    private let periodKey = "stock_chart_period_v1"
    private var timer: Timer?
    private var fetching = false
    private var candleTasks: [String: URLSessionDataTask] = [:]

    private init() {
        period = ChartPeriod(rawValue: UserDefaults.standard.string(forKey: periodKey) ?? "") ?? .day
        let saved = UserDefaults.standard.stringArray(forKey: defaultsKey) ?? []
        if UserDefaults.standard.object(forKey: defaultsKey) == nil {
            watchlist = ["sh000001", "sz399001", "usAAPL"]
        } else {
            var seen = Set<String>()
            watchlist = saved.compactMap { Self.normalize($0) }
                .filter { seen.insert($0).inserted }
                .prefix(8)
                .map { $0 }
        }
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
                      let text = String(data: data, encoding: String.Encoding(rawValue: CFStringConvertEncodingToNSStringEncoding(CFStringEncoding(CFStringEncodings.GB_18030_2000.rawValue)))) {
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
                        self.quotes = values.filter { q in self.watchlist.contains { $0.lowercased() == q.code.lowercased() } }
                        self.lastUpdated = Date()
                        self.status = "已更新 \(values.count) 只 · \(Self.timeText(Date()))"
                        self.sendLatest()
                        self.fetchCandles()
                    }
                case .failure(let error):
                    self.status = "行情更新失败：\(error.localizedDescription)"
                }
            }
        }.resume()
    }

    @discardableResult
    func add(_ raw: String) -> Bool {
        guard watchlist.count < 8, let code = Self.normalize(raw) else { return false }
        guard !watchlist.contains(code) else { return false }
        watchlist.append(code)
        save()
        quotes.removeAll()
        refresh()
        return true
    }

    func remove(_ code: String) {
        let key = (Self.normalize(code) ?? code).lowercased()
        watchlist.removeAll { $0.lowercased() == key }
        quotes.removeAll { $0.code.lowercased() == key }
        candles.removeValue(forKey: key.uppercased())
        candleTasks.filter { $0.key.hasSuffix(":" + key) }.forEach { $0.value.cancel() }
        candleTasks = candleTasks.filter { !$0.key.hasSuffix(":" + key) }
        status = watchlist.isEmpty ? "自选列表为空" : "已删除 \(key.uppercased())"
        save()
        sendLatest()
        guard !watchlist.isEmpty else { return }
        refresh()
    }

    /// Re-send the last successful payload after authentication or reconnect.
    func sendLatest() {
        guard BleLink.shared.isReady else { return }
        var parts = ["#STK", String(min(quotes.count, 8))]
        for q in quotes.prefix(8) {
            parts.append(q.code.uppercased())
            parts.append(String(format: "%.2f", q.price))
            parts.append(String(format: "%+.2f", q.changePct))
        }
        BleLink.shared.send(parts.joined(separator: ",") + (quotes.isEmpty ? "," : ""))
        for (i, q) in quotes.prefix(8).enumerated() {
            TextImage.send(slot: 13 + i, q.name, maxW: 180, size: 22, minSize: 16)
            sendCandles(q.code)
        }
    }

    func select(_ period: ChartPeriod) {
        guard self.period != period else { return }
        self.period = period
        UserDefaults.standard.set(period.rawValue, forKey: periodKey)
        candleTasks.values.forEach { $0.cancel() }
        candleTasks.removeAll()
        candles.removeAll()
        status = period == .day ? "正在更新日 K…" : "正在更新 1 分 K…"
        sendLatest()
        if !quotes.isEmpty { fetchCandles() }
    }

    private func fetchCandles() {
        let period = self.period
        for quote in quotes {
            let code = quote.code.lowercased()
            let taskKey = "\(period.rawValue):\(code)"
            if period == .minute && !Self.supportsMinuteK(code) {
                fetchMinuteFallback(quote, taskKey: taskKey)
                continue
            }
            let chartCode = Self.chartCode(code, period: period)
            guard let url = Self.chartURL(chartCode, period: period) else { continue }
            candleTasks.removeValue(forKey: taskKey)?.cancel()
            let task = URLSession.shared.dataTask(with: url) { [weak self] data, _, _ in
                var values: [Candle] = []
                if let data = data,
                   let json = try? JSONSerialization.jsonObject(with: data) as? [String: Any],
                   let all = json["data"] as? [String: Any], let item = all[chartCode] as? [String: Any],
                   let rows = (period == .day ? (item["qfqday"] ?? item["day"]) : item["m1"]) as? [[Any]] {
                    values = rows.suffix(24).compactMap { row in
                        guard row.count >= 5,
                              let o = Double(String(describing: row[1])),
                              let c = Double(String(describing: row[2])),
                              let h = Double(String(describing: row[3])),
                              let l = Double(String(describing: row[4])),
                              [o,c,h,l].allSatisfy({ $0.isFinite && $0 > 0 }), h >= max(o,c), l <= min(o,c) else { return nil }
                        return Candle(open:o, close:c, high:h, low:l)
                    }
                }
                DispatchQueue.main.async {
                    guard let self = self,
                          self.watchlist.contains(where: { $0.lowercased() == code }) else { return }
                    self.candleTasks.removeValue(forKey: taskKey)
                    self.candles[quote.code] = values
                    self.sendCandles(quote.code)
                }
            }
            candleTasks[taskKey] = task
            task.resume()
        }
    }

    private func fetchMinuteFallback(_ quote: Quote, taskKey: String) {
        let code = quote.code.lowercased()
        let queryCode = String(code.prefix(2)) + quote.code.dropFirst(2)
        guard let url = URL(string: "https://ifzq.gtimg.cn/appstock/app/minute/query?code=\(queryCode)") else { return }
        candleTasks.removeValue(forKey: taskKey)?.cancel()
        let task = URLSession.shared.dataTask(with: url) { [weak self] data, _, _ in
            var values: [Candle] = []
            if let data = data,
               let json = try? JSONSerialization.jsonObject(with: data) as? [String: Any],
               let all = json["data"] as? [String: Any],
               let item = all[queryCode] as? [String: Any],
               let container = item["data"] as? [String: Any],
               let rows = container["data"] as? [String] {
                let points = rows.compactMap { row -> Double? in
                    row.split(separator: " ").dropFirst().first.flatMap { Double($0) }
                }.filter { $0.isFinite && $0 > 0 }
                var previous = points.first ?? quote.price
                for price in points.suffix(24) {
                    values.append(Candle(open: previous, close: price,
                                         high: max(previous, price), low: min(previous, price)))
                    previous = price
                }
            }
            DispatchQueue.main.async {
                guard let self = self,
                      self.watchlist.contains(where: { $0.lowercased() == code }) else { return }
                self.candleTasks.removeValue(forKey: taskKey)
                self.candles[quote.code] = values
                self.sendCandles(quote.code)
            }
        }
        candleTasks[taskKey] = task
        task.resume()
    }

    private func sendCandles(_ code: String) {
        guard BleLink.shared.isReady else { return }
        let values = Array((candles[code] ?? []).suffix(12))
        var parts = ["#SK", code, period.protocolCode, String(values.count)]
        let low = values.map(\.low).min() ?? 0
        let high = values.map(\.high).max() ?? 1
        let span = max(high - low, 0.000001)
        for c in values {
            parts += [c.open,c.close,c.high,c.low].map { String(Int((($0-low)/span*100).rounded())) }
        }
        BleLink.shared.send(parts.joined(separator: ","))
    }

    private static func supportsMinuteK(_ code: String) -> Bool {
        code.hasPrefix("sh") || code.hasPrefix("sz") || code.hasPrefix("bj")
    }

    private static func chartCode(_ code: String, period: ChartPeriod) -> String {
        if period == .day && code.hasPrefix("us") && !code.contains(".") {
            return code + ".OQ"
        }
        return code
    }

    private static func chartURL(_ code: String, period: ChartPeriod) -> URL? {
        let path: String
        switch period {
        case .day:
            path = "https://ifzq.gtimg.cn/appstock/app/fqkline/get?param=\(code),day,,,24,qfq"
        case .minute:
            path = "https://ifzq.gtimg.cn/appstock/app/kline/mkline?param=\(code),m1,,24"
        }
        return URL(string: path)
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
            found[key] = Quote(code: key.uppercased(), name: fields[1], price: price, changePct: pct)
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
