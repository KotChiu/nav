import Foundation

enum MapStyles {
    static func original(_ raw: [UInt32]) -> (indices: [UInt8], palette: [Int]) {
        var counts: [Int: Int] = [:]
        for c in raw { let rgb = Int((c >> 19) & 31) << 11 | Int((c >> 10) & 63) << 5 | Int((c >> 3) & 31); counts[rgb, default: 0] += 1 }
        let ranked = counts.keys.sorted { counts[$0]! == counts[$1]! ? $0 < $1 : counts[$0]! > counts[$1]! }
        var palette = Array(ranked.prefix(16)); while palette.count < 16 { palette.append(palette.last ?? 0) }
        func delta(_ a: Int, _ b: Int) -> Int {
            let r = (a >> 11) - (b >> 11), g = ((a >> 5) & 63) - ((b >> 5) & 63), bl = (a & 31) - (b & 31)
            return r*r*4 + g*g + bl*bl*4
        }
        var lookup: [Int: UInt8] = [:]
        for c in ranked { lookup[c] = UInt8(palette.indices.min { delta(c, palette[$0]) < delta(c, palette[$1]) }!) }
        return (raw.map { c in lookup[Int((c >> 19) & 31) << 11 | Int((c >> 10) & 63) << 5 | Int((c >> 3) & 31)]! }, palette)
    }
    /// Keep the same 2.4 km extent. The 600² fallback fits even with one
    /// encoded byte per pixel, so a dense original map can always be sent.
    static func originalPacket(_ raw: [UInt32], frameId: Int, x0: Int, y0: Int,
                               shouldCancel: () -> Bool) throws -> Data {
        let fullSize = RasterPacket.size
        guard raw.count == fullSize * fullSize else {
            throw MapProcessingError.invalid("高德原版底图尺寸不正确")
        }
        let colors = original(raw)
        for size in [fullSize, 800, 600] {
            if shouldCancel() { throw MapProcessingError.cancelled }
            let pixels: [UInt8]
            if size == fullSize {
                pixels = colors.indices
            } else {
                pixels = (0..<(size * size)).map { index in
                    let x = (2 * (index % size) + 1) * fullSize / (2 * size)
                    let y = (2 * (index / size) + 1) * fullSize / (2 * size)
                    return colors.indices[y * fullSize + x]
                }
            }
            let packet = MapGeo.encodeRaster(frameId: frameId, x0: x0, y0: y0, w: size, h: size,
                mmPerPx: 2_400_000 / size, idx: pixels, pal: colors.palette)
            if RasterPacket.valid(packet, frame: frameId, x: x0, y: y0) { return packet }
        }
        throw MapProcessingError.invalid("高德原版底图编码失败")
    }

    static func minimal(_ raw: [UInt32], size: Int) -> [UInt8] {
        let mask: [UInt8] = raw.map { c in
            let r = Int((c >> 16) & 255), g = Int((c >> 8) & 255), b = Int(c & 255)
            return (r >= 254 && g >= 254 && b >= 254) || (r >= 200 && g >= 125 && g <= 245 && b+25 < g && r >= g && r-b >= 45) ? 1 : 0
        }
        return MapGeo.cleanRaster(mask, w: size, h: size).map { $0 == 1 ? 1 : 0 }
    }
}
