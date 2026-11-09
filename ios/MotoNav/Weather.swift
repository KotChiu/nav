import Foundation
import CoreLocation
import SwiftUI

/// Same endpoints, units and screen protocol as Android QWeatherClient.
final class Weather: ObservableObject {
    static let shared = Weather()
    @Published var info = "在圆屏切换到天气页查询"
    @Published var detail = "尚未查询天气"
    var host: String { UserDefaults.standard.string(forKey: "qw_host") ?? "" }
    var key: String { UserDefaults.standard.string(forKey: "qw_key") ?? "" }
    private var generation = 0
    private var busy = false
    private var inFlightAt = Date.distantPast       // busy 卡死自愈
    private var locationRetries = 0
    private var lastAt = Date.distantPast
    private var cacheLifetime: TimeInterval = 600
    private var retryAt = Date.distantPast
    private var failStreak = 0
    private var messages: [String] = []
    private var images: [(Int, TextImage.Img?)] = []
    private var slow: [Int: [String: Any]] = [:]    // 降水 / 预警：半小时才重新联网
    private var slowAt = Date.distantPast
    private let geocoder = CLGeocoder()
    private let store = UserDefaults.standard
    private static let backoff: [TimeInterval] = [30, 60, 120, 300, 300]

    init() { restore() }

    /// 冷启动先上屏上一次成功的 #W（温度/预报），别让圆屏一直停在 WEATHER FROM PHONE。
    /// 写入 BleLink.lastState，连接就绪时会自动补发，所以这里不要求蓝牙已连上。
    private func restore() {
        guard let saved = store.stringArray(forKey: "wx_messages"), !saved.isEmpty,
              store.double(forKey: "wx_lastAt") > 0 else { return }
        messages = saved
        lastAt = Date(timeIntervalSince1970: store.double(forKey: "wx_lastAt"))
        info = "上次天气数据 \(Self.clock(lastAt))，正在更新…"
        DispatchQueue.main.async { [weak self] in
            guard let self = self else { return }
            self.repushSummary()          // 先把温度/预报写上屏
            self.request(force: true)     // 再补齐详情图和最新数据
        }
    }
    private func persist() {
        store.set(messages, forKey: "wx_messages")
        store.set(lastAt.timeIntervalSince1970, forKey: "wx_lastAt")
    }
    private static func clock(_ date: Date) -> String {
        let f = DateFormatter(); f.dateFormat = "HH:mm"; return f.string(from: date)
    }
    /// 只重发两条文字（更新固件的 wxAt，抑制 30s 轮询），不重复推 7 张详情图。
    private func repushSummary() { for message in messages { BleLink.shared.send(message) } }

    func save(host: String, key: String) throws {
        let normalized = host.trimmingCharacters(in: .whitespacesAndNewlines)
            .replacingOccurrences(of: "https://", with: "").trimmingCharacters(in: CharacterSet(charactersIn: "/")).lowercased()
        guard normalized.range(of: #"^[a-z0-9-]+(?:\.[a-z0-9-]+)*\.qweatherapi\.com$"#, options: .regularExpression) != nil,
              !key.isEmpty, !key.contains(where: { $0.isWhitespace }) else {
            throw NSError(domain: "Weather", code: 1, userInfo: [NSLocalizedDescriptionKey: "请填写 xxxx.qweatherapi.com API Host 和有效 API Key"])
        }
        UserDefaults.standard.set(normalized, forKey: "qw_host")
        UserDefaults.standard.set(key, forKey: "qw_key")
        generation += 1; busy = false; messages = []; images = []; lastAt = .distantPast
        retryAt = .distantPast; failStreak = 0; slow = [:]; slowAt = .distantPast
        store.removeObject(forKey: "wx_messages"); store.removeObject(forKey: "wx_lastAt")
        info = "配置已保存，正在查询天气"; detail = info
        clearScreen()
        locationRetries = 0
        request(force: true)
    }
    private func clearScreen() {
        BleLink.shared.clearWeather()
    }
    /// 天气按 1km 网格查询、10 分钟缓存，位置不需要秒级新鲜。
    /// 骑车锁屏时 CoreLocation（WhenInUse）会停更，所以导航/巡航中优先用高德的实时定位。
    private func currentLocation() -> (lat: Double, lng: Double, stale: Bool)? {
        let navi = NaviBridge.shared
        if (navi.navigating || navi.cruising), let p = navi.myPosition() { return (p.lat, p.lng, false) }
        if let l = LocationService.shared.location, l.horizontalAccuracy >= 0 {
            let age = abs(l.timestamp.timeIntervalSinceNow)
            if age < 600 { return (l.coordinate.latitude, l.coordinate.longitude, false) }
            if age < 3600 { return (l.coordinate.latitude, l.coordinate.longitude, true) }
        }
        if let p = navi.myPosition() { return (p.lat, p.lng, true) }
        return nil
    }

    private func noteFailure(_ reason: String) {
        failStreak += 1
        retryAt = Date().addingTimeInterval(Self.backoff[min(failStreak - 1, Self.backoff.count - 1)])
        let tail = messages.isEmpty ? "暂无缓存数据"
            : "继续显示 \(Self.clock(lastAt)) 的数据，\(Int(retryAt.timeIntervalSinceNow)) 秒后重试"
        info = "天气更新失败（\(reason)）；\(tail)"
        detail = info
        if messages.isEmpty { clearScreen() }
        else { repushSummary() }   // 刷新固件 wxAt，避免它每 30 秒追问一次
    }

    func request(force: Bool = false) {
        guard !host.isEmpty, !key.isEmpty else { info = "请先配置和风天气"; clearScreen(); return }
        // CLGeocoder / URLSession 偶尔不回调，busy 会永久堵住后续所有请求。
        if busy && Date().timeIntervalSince(inFlightAt) > 25 { busy = false }
        guard !busy else { return }
        if !force && !messages.isEmpty && Date().timeIntervalSince(lastAt) < cacheLifetime { repushSummary(); return }
        guard Date() >= retryAt else { return }
        guard let p = currentLocation() else {
            info = "等待手机定位，请稍后重试"
            LocationService.shared.start()
            if locationRetries < 5 {
                locationRetries += 1
                let expectedGeneration = generation
                DispatchQueue.main.asyncAfter(deadline: .now() + 3) { [weak self] in
                    guard let self = self, self.generation == expectedGeneration else { return }
                    self.request(force: force)
                }
            }
            return
        }
        locationRetries = 0
        busy = true; inFlightAt = Date(); info = "正在查天气…"
        generation += 1
        let id = generation, accountHost = host, accountKey = key
        let loc = CLLocation(latitude: p.lat, longitude: p.lng)
        geocoder.reverseGeocodeLocation(loc, preferredLocale: Locale(identifier: "zh_CN")) { [weak self] marks, _ in
            DispatchQueue.main.async { [weak self] in
                guard let self = self, self.generation == id else { self?.busy = false; return }
                let mark = marks?.first
                let place = mark?.subLocality ?? mark?.locality ?? mark?.administrativeArea ?? "当前位置"
                Task { await self.fetch(loc, place: place, stale: p.stale, host: accountHost, key: accountKey, id: id) }
            }
        }
    }
    @MainActor private func fetch(_ loc: CLLocation, place: String, stale: Bool,
                                  host: String, key: String, id: Int) async {
        defer { busy = false }
        let en = Locale(identifier: "en_US_POSIX")
        let point = String(format: "%.2f/%.2f", locale: en, loc.coordinate.latitude, loc.coordinate.longitude)
        let location = String(format: "%.2f%%2C%.2f", locale: en, loc.coordinate.longitude, loc.coordinate.latitude)
        let all = ["/weather/v1/current/\(point)?lang=zh&localTime=true",
                   "/weather/v1/hourly/\(point)?hours=6&lang=zh&localTime=true",
                   "/v7/minutely/5m?location=\(location)&lang=zh",
                   "/airquality/v1/current/\(point)?lang=zh",
                   "/weatheralert/v1/current/\(point)?lang=zh&localTime=true"]
        // 免费订阅每天 1000 次：常规刷新只打 3 个核心端点，降水和预警半小时才联网一次。
        let slowFresh = Date().timeIntervalSince(slowAt) < 1800
        let indexes = slowFresh ? [0, 1, 3] : [0, 1, 2, 3, 4]
        let labels = ["实况", "逐小时", "降水", "空气", "预警"]
        let fetched = await withTaskGroup(of: (Int, [String: Any]?, String?).self) { group in
            for i in indexes {
                let path = all[i]
                group.addTask {
                    try? await Task.sleep(nanoseconds: UInt64(i) * 120_000_000)   // 错峰，避开瞬时 QPS 限制
                    do { return (i, try await QWeatherHTTP.get(host: host, key: key, path: path), nil) }
                    catch { return (i, nil, error.localizedDescription) }
                }
            }
            var values: [(Int, [String: Any]?, String?)] = []
            for await value in group { values.append(value) }
            return values.sorted { $0.0 < $1.0 }
        }
        var results: [[String: Any]?] = Array(repeating: nil, count: 5)
        var notes: [String] = []
        for item in fetched {
            if let json = item.1 {
                results[item.0] = json
                if item.0 == 2 || item.0 == 4 { slow[item.0] = json }
            } else { notes.append("\(labels[item.0])：\(item.2 ?? "请求失败")") }
        }
        // 即使慢端点没订到也记时间，避免每次刷新都白白重试这两个接口。
        if !slowFresh { slowAt = Date() }
        for i in [2, 4] {
            if results[i] == nil, let cached = slow[i] { results[i] = cached }
        }
        guard generation == id else { return }
        if results[0] == nil {
            noteFailure(notes.isEmpty ? "实况接口未响应" : notes.joined(separator: "；"))
            return
        }
        guard generation == id else { return }
        failStreak = 0; retryAt = .distantPast
        do { try build(results, place: place, notes: notes, stale: stale); persist() }
        catch { noteFailure("天气数据无效") }
    }
    private func build(_ r: [[String: Any]?], place: String, notes: [String], stale: Bool) throws {
        func dict(_ v: Any?) -> [String: Any] { v as? [String: Any] ?? [:] }
        func number(_ v: Any?) -> Double? {
            let n = (v as? NSNumber)?.doubleValue ?? Double(v as? String ?? "")
            return n?.isFinite == true ? n : nil
        }
        func integer(_ v: Any?, fallback: Int = -1) -> Int {
            guard let n = number(v), abs(n) < 1_000_000 else { return fallback }; return Int(n.rounded())
        }
        let now = r[0] ?? [:], text = dict(now["condition"])["text"] as? String ?? ""
        guard let temp = number(dict(now["temperature"])["value"]), (-100...70).contains(temp) else { throw URLError(.cannotParseResponse) }
        let night = Calendar.current.component(.hour, from: Date()) < 6 || Calendar.current.component(.hour, from: Date()) >= 19
        let wind = dict(now["wind"]), direction = integer(dict(wind["direction"])["degree"])
        let windName = direction >= 0 && direction < 360 ? ["北", "东北", "东", "东南", "南", "西南", "西", "西北"][((direction + 22) / 45) % 8] + "风" : "风向未知"
        let humidity = number(now["humidity"])
        let windText = windName + " \(integer(wind["scale"]))级 · " + (humidity != nil && (0...1).contains(humidity!) ? "湿度 \(Int((humidity! * 100).rounded()))%" : "湿度未知")
        let referenceTime = Date()
        let forecast = QWeatherForecast.parse(r[1], now: referenceTime, icon: { Self.icon($0, night: $1) })
        let hours = forecast.hours.map { $0.wire }
        let allNotes = notes + (forecast.note.map { [$0] } ?? [])
        // 逐小时接口没订阅时原来把缓存压到 30 秒，固件每次追问都会真的联网，额度很快打光。
        cacheLifetime = 600
        let rain = r[2]?["summary"] as? String ?? "降水预报暂不可用"
        let bars = (r[2]?["minutely"] as? [[String: Any]] ?? []).prefix(24).map { item -> Int in
            guard let n = number(item["precip"]), n >= 0 else { return -1 }; return Int(min(9999, (n * 100).rounded()))
        }
        let indexes = r[3]?["indexes"] as? [[String: Any]] ?? []
        let index = indexes.first { $0["code"] as? String == "cn-mee" } ?? indexes.first ?? [:]
        let aqi = integer(index["aqi"])
        let air = aqi >= 0 ? "\(index["name"] as? String ?? "AQI") \(index["aqiDisplay"] as? String ?? String(aqi)) · \(index["category"] as? String ?? "")" : "空气质量暂不可用"
        let alerts = (r[4]?["alerts"] as? [[String: Any]] ?? []).filter {
            dict($0["messageType"])["code"] as? String != "cancel" && (QWeatherForecast.timestamp($0["expireTime"] as? String ?? "") ?? .distantFuture) > Date()
        }
        func level(_ a: [String: Any]) -> Int { ["minor": 1, "moderate": 2, "severe": 3, "extreme": 4][a["severity"] as? String ?? ""] ?? 1 }
        let top = alerts.max { level($0) < level($1) }
        let confirmed = r[4]?["alerts"] is [[String: Any]] || dict(r[4]?["metadata"])["zeroResult"] as? Bool == true
        let title = top?["headline"] as? String ?? (confirmed ? "当前位置暂无有效预警" : "预警信息暂不可用")
        let body = top?["description"] as? String ?? "", advice = top?["instruction"] as? String ?? ""
        let f = DateFormatter(); f.dateFormat = "HH:mm"
        // Stable Java-compatible hash: the firmware uses it to distinguish warnings.
        var hash: Int32 = 0
        for c in title.utf16 { hash = hash &* 31 &+ Int32(c) }
        messages = ["#WX,\(aqi),\(confirmed ? alerts.count : -1),\(top.map(level) ?? 0),\(bars.count),\(hash & 0x7fffffff)" + bars.map { ",\($0)" }.joined(),
                    "#W,\(Self.icon(text, night: night)),\(Int(temp.rounded())),\(f.string(from: Date())),H,\(hours.count)" + hours.map { ",\($0)" }.joined()]
        images = [(3, "\(place) · \(text)"), (4, windText), (7, rain), (8, air), (9, title), (10, String(body.prefix(60))), (11, String(advice.prefix(60)))].map {
            ($0.0, TextImage.render($0.1, maxW: 330, size: 24, minSize: 18))
        }
        var sources = ["和风天气 QWeather · https://www.qweather.com/", "https://developer.qweather.com/attribution.html"]
        for item in r { sources += dict(item?["metadata"])["attributions"] as? [String] ?? [] }
        detail = (["\(place) · \(text) \(Int(temp.rounded()))°", windText, rain, air] + alerts.map { [ $0["headline"], $0["description"], $0["instruction"] ].compactMap { $0 as? String }.joined(separator: "\n") } + [title] + allNotes + Array(Set(sources)).sorted() + ["预警可能延迟，请以官方最新发布为准。更新：\(f.string(from: Date()))"]).joined(separator: "\n\n")
        info = (stale ? "位置较旧 · " : "") + "\(place) \(text) \(Int(temp.rounded()))°；逐小时 \(hours.count) 个"
            + (allNotes.isEmpty ? "" : "；" + allNotes.joined(separator: "；"))
        lastAt = Date(); push()
    }
    private func push() {
        for (slot, im) in images { BleLink.shared.sendImage(slot: slot, w: im?.w ?? 0, h: im?.h ?? 0, data: im?.data ?? Data()) }
        for message in messages { BleLink.shared.send(message) }
    }
    static func icon(_ text: String, night: Bool) -> Int {
        if text.contains("雷") { return 5 }
        if text.contains("冰雹") || (text.contains("雪") && text.contains("雨")) { return 8 }
        if text.contains("雪") { return 6 }
        if text.contains("暴雨") || text.contains("大雨") { return 4 }
        if text.contains("雨") { return 3 }
        if text.contains("雾") || text.contains("霾") { return 7 }
        if text.contains("沙") || text.contains("尘") { return 9 }
        if text.contains("阴") { return 2 }
        if text.contains("云") { return night ? 11 : 1 }
        if text.contains("晴") { return night ? 10 : 0 }
        return 2
    }
}

/// Disallow redirects so an account credential cannot be forwarded to another host.
final class QWeatherHTTP: NSObject, URLSessionTaskDelegate {
    static let shared = QWeatherHTTP()
    private lazy var session = URLSession(configuration: .ephemeral, delegate: self, delegateQueue: nil)
    func urlSession(_ session: URLSession, task: URLSessionTask, willPerformHTTPRedirection response: HTTPURLResponse, newRequest request: URLRequest, completionHandler: @escaping (URLRequest?) -> Void) { completionHandler(nil) }
    /// 只对传输类错误重试一次；402/429 是额度问题，重试只会更快耗尽配额。
    static func get(host: String, key: String, path: String) async throws -> [String: Any] {
        do { return try await once(host: host, key: key, path: path) }
        catch let error as URLError {
            switch error.code {
            case .timedOut, .cannotConnectToHost, .cannotFindHost, .networkConnectionLost, .notConnectedToInternet:
                try? await Task.sleep(nanoseconds: 400_000_000)
                return try await once(host: host, key: key, path: path)
            default: throw error
            }
        }
    }
    private static func once(host: String, key: String, path: String) async throws -> [String: Any] {
        var request = URLRequest(url: URL(string: "https://\(host)\(path)")!, timeoutInterval: 10)
        request.setValue(key, forHTTPHeaderField: "X-QW-Api-Key")
        let (data, response) = try await shared.session.data(for: request)
        let status = (response as? HTTPURLResponse)?.statusCode ?? 0
        guard status == 200 else { throw QWeatherRequestError.http(status) }
        guard let json = try JSONSerialization.jsonObject(with: data) as? [String: Any] else { throw QWeatherRequestError.invalidData }
        if let code = json["code"], String(describing: code) != "200" {
            let safeCode = String(describing: code).filter { $0.isNumber }.prefix(8)
            throw QWeatherRequestError.service(safeCode.isEmpty ? "未知" : String(safeCode))
        }
        return json
    }
}

struct WeatherSettingsView: View {
    @ObservedObject private var weather = Weather.shared
    @State private var host = Weather.shared.host
    @State private var key = Weather.shared.key
    @State private var error = ""
    var body: some View {
        DisclosureGroup("和风天气 · 可选配置") {
            Text("填写 API Host 和 API Key 后，可在圆屏查询当前天气和逐小时预报。")
                .font(.caption).foregroundColor(ContentView.sub)
            TextField("API Host：xxxx.qweatherapi.com", text: $host).textInputAutocapitalization(.never).autocorrectionDisabled()
            SecureField("API Key", text: $key).textInputAutocapitalization(.never).autocorrectionDisabled()
            Button("保存天气配置") {
                do { try weather.save(host: host, key: key.trimmingCharacters(in: .whitespacesAndNewlines)); error = "" }
                catch { self.error = error.localizedDescription }
            }
            if !error.isEmpty { Text(error).foregroundColor(.orange) }
            Button("刷新天气") { weather.request(force: true) }
            Text(weather.info).font(.caption)
            Text(weather.detail).font(.caption).textSelection(.enabled)
            Link("和风天气来源说明", destination: URL(string: "https://developer.qweather.com/attribution.html")!)
        }.tint(ContentView.cyan)
    }
}

extension Array {
    subscript(safe i: Int) -> Element? { indices.contains(i) ? self[i] : nil }
}
