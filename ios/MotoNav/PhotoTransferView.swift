import SwiftUI
import PhotosUI
import UIKit

struct PhotoTransferView: View {
    @ObservedObject private var ble = BleLink.shared
    @ObservedObject private var navigation = NaviBridge.shared
    @ObservedObject private var webNavigation = WebMotorSession.shared
    @State private var picker = false
    @State private var image: UIImage?
    @State private var status = "选择照片后，点发送同步到圆屏"
    @AppStorage("photo_transfer_id") private var sequence = 0
    var body: some View {
        VStack(alignment: .leading, spacing: 10) {
            Text("固定图片 · 2.1 圆屏").bold().foregroundColor(.white)
            if let image = image {
                Image(uiImage: image).resizable().scaledToFill().frame(width: 110, height: 110).clipShape(Circle())
            }
            HStack {
                Button("选择照片") { picker = true }
                Button("发送到圆屏") { send() }
                    .disabled(image == nil || !ble.isReady || !ble.photoSupported || navigation.navigating || navigation.cruising || webNavigation.running)
                Button("清除") { ble.photoTransferStatus = "已请求清除圆屏图片"; ble.sendOnce("#PHOTO,CLEAR"); status = "已请求清除圆屏图片" }
                    .disabled(!ble.isReady || !ble.photoSupported)
            }.buttonStyle(.borderless)
            if ble.isReady && !ble.photoSupported {
                Button("查询图片支持") { ble.sendOnce("#PHOTO?") }
                Text("当前圆屏尚未确认支持固定图片，请使用新版 2.1 固件。")
            }
            Text(ble.photoTransferStatus.isEmpty ? status : ble.photoTransferStatus).font(.caption)
            Text("居中裁成圆屏画面，232×232 像素。传输约 105 KB，可能需要数秒。在圆屏左右滑动到照片页查看。导航、巡航时不可发送；导航、来电、配对优先。图片仅存圆屏内存，断开手机仍保留，重启后需重新发送。")
                .font(.caption2).foregroundColor(ContentView.sub)
        }
        .onAppear { if ble.isReady { ble.sendOnce("#PHOTO?") } }
        .sheet(isPresented: $picker) { PhotoPicker { image = $0 } }
    }
    private func send() {
        guard ble.isReady, ble.photoSupported, !navigation.navigating, !navigation.cruising, !webNavigation.running, let image = image,
              let data = PhotoTransferEncoder.encode(image) else { status = "图片处理失败"; return }
        sequence = (sequence + 1) & 255
        ble.photoTransferStatus = "图片正在传输，请稍候"
        ble.sendBulk(type: 6, id: sequence, data: data)
        status = "图片已加入传输队列，请等待圆屏确认"
    }
}

enum PhotoTransferEncoder {
    static let pixels = 232
    static func encode(_ image: UIImage) -> Data? {
        let size = pixels
        guard image.size.width > 0, image.size.height > 0 else { return nil }
        var rgba = [UInt8](repeating: 0, count: size * size * 4)
        let ok = rgba.withUnsafeMutableBytes { bytes -> Bool in
            guard let ctx = CGContext(data: bytes.baseAddress, width: size, height: size, bitsPerComponent: 8,
                bytesPerRow: size * 4, space: CGColorSpaceCreateDeviceRGB(),
                bitmapInfo: CGImageAlphaInfo.premultipliedLast.rawValue | CGBitmapInfo.byteOrder32Big.rawValue) else { return false }
            ctx.setFillColor(UIColor.black.cgColor); ctx.fill(CGRect(x: 0, y: 0, width: size, height: size))
            ctx.translateBy(x: 0, y: CGFloat(size)); ctx.scaleBy(x: 1, y: -1)
            let scale = max(CGFloat(size) / image.size.width, CGFloat(size) / image.size.height)
            let w = image.size.width * scale, h = image.size.height * scale
            UIGraphicsPushContext(ctx)
            image.draw(in: CGRect(x: (CGFloat(size)-w)/2, y: (CGFloat(size)-h)/2, width: w, height: h))
            UIGraphicsPopContext(); return true
        }
        guard ok else { return nil }
        var data = Data([80,72,79,1,UInt8(size),0,UInt8(size),0])
        for i in stride(from: 0, to: rgba.count, by: 4) {
            let color = (UInt16(rgba[i] >> 3) << 11) | (UInt16(rgba[i+1] >> 2) << 5) | UInt16(rgba[i+2] >> 3)
            data.append(UInt8(color & 255)); data.append(UInt8(color >> 8))
        }
        return data.count == 8 + size * size * 2 ? data : nil
    }
}

private struct PhotoPicker: UIViewControllerRepresentable {
    let selected: (UIImage) -> Void
    func makeCoordinator() -> Coordinator { Coordinator(selected) }
    func makeUIViewController(context: Context) -> PHPickerViewController {
        var config = PHPickerConfiguration(); config.filter = .images; config.selectionLimit = 1
        let controller = PHPickerViewController(configuration: config); controller.delegate = context.coordinator
        return controller
    }
    func updateUIViewController(_ uiViewController: PHPickerViewController, context: Context) {}
    final class Coordinator: NSObject, PHPickerViewControllerDelegate {
        let selected: (UIImage) -> Void
        init(_ selected: @escaping (UIImage) -> Void) { self.selected = selected }
        func picker(_ picker: PHPickerViewController, didFinishPicking results: [PHPickerResult]) {
            picker.dismiss(animated: true)
            guard let provider = results.first?.itemProvider, provider.canLoadObject(ofClass: UIImage.self) else { return }
            let deliver = selected
            provider.loadObject(ofClass: UIImage.self) { object, _ in
                guard let image = object as? UIImage else { return }
                DispatchQueue.main.async { deliver(image) }
            }
        }
    }
}
