package com.example.motonav

import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.graphics.Color
import android.graphics.Matrix
import android.media.ExifInterface
import android.graphics.drawable.GradientDrawable
import android.net.Uri
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.view.Gravity
import android.view.ViewGroup
import android.widget.Button
import android.widget.ImageView
import android.widget.LinearLayout
import android.widget.TextView
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AppCompatActivity
import java.util.zip.CRC32

/**
 * Independent electronic badge/photo-upload page. The Android system picker
 * grants access only to the selected image, without broad photo permissions.
 * Wire format matches firmware/ui/BadgePacket.h.
 */
class BadgeActivity : AppCompatActivity() {
    private val handler = Handler(Looper.getMainLooper())
    private var selectedBitmap: Bitmap? = null
    private lateinit var preview: ImageView
    private lateinit var status: TextView

    private val choose = registerForActivityResult(ActivityResultContracts.GetContent()) { uri ->
        if (uri != null) {
            try {
                val photo = decode(uri)
                selectedBitmap = photo
                preview.setImageBitmap(photo)
            } catch (_: Exception) {
                selectedBitmap = null
                status.text = "无法打开图片，请重新选择"
            }
        }
    }

    private fun dp(px: Int) = (resources.displayMetrics.density * px).toInt()
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        supportActionBar?.hide()
        window.statusBarColor = Color.rgb(13, 11, 25)
        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            gravity = Gravity.CENTER_HORIZONTAL
            setPadding(dp(22), dp(28), dp(22), dp(22))
            setBackgroundColor(Color.rgb(13, 11, 25))
        }
        fun text(value: String, sp: Float, color: Int): TextView = TextView(this).apply {
            text = value; textSize = sp; setTextColor(color); gravity = Gravity.CENTER
            setPadding(dp(8), dp(8), dp(8), dp(8))
        }
        root.addView(text("电子吧唧", 27f, Color.rgb(45, 228, 240)))
        root.addView(text("从手机选择照片，裁剪成圆形预览，再通过 BLE 上传至 2.1 英寸圆屏", 13f, Color.LTGRAY))
        preview = ImageView(this).apply {
            scaleType = ImageView.ScaleType.CENTER_CROP
            val oval = GradientDrawable().apply {
                shape = GradientDrawable.OVAL
                setColor(Color.BLACK)
                setStroke(dp(2), Color.rgb(45, 228, 240))
            }
            background = oval
            clipToOutline = true
        }
        root.addView(preview, LinearLayout.LayoutParams(dp(260), dp(260)).apply {
            topMargin = dp(28); bottomMargin = dp(24)
        })
        status = text("", 13f, Color.LTGRAY)
        root.addView(status)
        root.addView(Button(this).apply {
            text = "选择照片"
            setOnClickListener { choose.launch("image/*") }
        }, LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, dp(54)))
        root.addView(Button(this).apply {
            text = "上传到圆屏"
            setOnClickListener {
                val photo = selectedBitmap
                if (photo == null) status.text = "请先选择照片"
                else BadgeTransfer.upload(photo)
            }
        }, LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, dp(54)).apply {
            topMargin = dp(10)
        })
        root.addView(text("上传请在停车时进行。图片最长 240×240，手机居中裁剪并缩放；断电后是否保留以设备存储状态为准。", 12f, Color.GRAY))
        setContentView(root)
    }
    private val refresh = object : Runnable {
        override fun run() {
            status.text = BadgeTransfer.status + if (!BleLink.badgeSupported)
                "\n未检测到电子吧唧固件（需要连接认证后才能上传）" else ""
            handler.postDelayed(this, 400)
        }
    }
    override fun onResume() { super.onResume(); handler.post(refresh) }
    override fun onPause() { handler.removeCallbacks(refresh); super.onPause() }

    private fun decode(uri: Uri): Bitmap {
        val bounds = BitmapFactory.Options().apply { inJustDecodeBounds = true }
        contentResolver.openInputStream(uri)?.use { BitmapFactory.decodeStream(it, null, bounds) }
        require(bounds.outWidth > 0 && bounds.outHeight > 0) { "Invalid image" }
        var sample = 1
        while (maxOf(bounds.outWidth, bounds.outHeight) / sample > 1200) sample *= 2
        val opts = BitmapFactory.Options().apply { inSampleSize = sample }
        val raw = contentResolver.openInputStream(uri)?.use {
            BitmapFactory.decodeStream(it, null, opts)
        } ?: error("Cannot read image")
        val orientation = try {
            contentResolver.openInputStream(uri)?.use {
                ExifInterface(it).getAttributeInt(ExifInterface.TAG_ORIENTATION,
                    ExifInterface.ORIENTATION_NORMAL)
            } ?: ExifInterface.ORIENTATION_NORMAL
        } catch (_: Exception) { ExifInterface.ORIENTATION_NORMAL }
        val angle = when (orientation) {
            ExifInterface.ORIENTATION_ROTATE_90 -> 90f
            ExifInterface.ORIENTATION_ROTATE_180 -> 180f
            ExifInterface.ORIENTATION_ROTATE_270 -> 270f
            else -> 0f
        }
        if (angle == 0f) return raw
        return Bitmap.createBitmap(raw, 0, 0, raw.width, raw.height,
            Matrix().apply { postRotate(angle) }, true)
    }

    companion object {
        const val SIZE = 240
        fun encode(bitmap: Bitmap): ByteArray {
            val side = minOf(bitmap.width, bitmap.height)
            require(side > 0)
            val square = Bitmap.createBitmap(bitmap, (bitmap.width - side) / 2,
                (bitmap.height - side) / 2, side, side)
            val scaled = Bitmap.createScaledBitmap(square, SIZE, SIZE, true)
            val argb = IntArray(SIZE * SIZE)
            scaled.getPixels(argb, 0, SIZE, 0, 0, SIZE, SIZE)
            val out = ByteArray(12 + SIZE * SIZE * 2)
            out[0] = 'B'.code.toByte(); out[1] = 'D'.code.toByte()
            out[2] = 'G'.code.toByte(); out[3] = '1'.code.toByte()
            out[4] = SIZE.toByte(); out[5] = (SIZE shr 8).toByte()
            out[6] = SIZE.toByte(); out[7] = (SIZE shr 8).toByte()
            for (i in argb.indices) {
                val c = argb[i]
                val red = (c ushr 16) and 0xFF
                val green = (c ushr 8) and 0xFF
                val blue = c and 0xFF
                val v = ((red and 0xF8) shl 8) or ((green and 0xFC) shl 3) or (blue ushr 3)
                out[12 + i*2] = v.toByte()
                out[13 + i*2] = (v ushr 8).toByte()
            }
            val crc = CRC32()
            crc.update(out, 12, out.size - 12)
            val check = crc.value
            for (i in 0..3) out[8+i] = (check ushr (i*8)).toByte()
            return out
        }
    }
}

/** Keep transfer status independent of Activity lifecycle and navigation UI. */
object BadgeTransfer {
    @Volatile var status: String = "选择照片后上传到圆屏"
    @Volatile private var pendingId = -1
    @Volatile private var encoding = false
    private var seq = 0
    private val handler = Handler(Looper.getMainLooper())

    fun upload(photo: Bitmap) {
        if (!BleLink.isConnected || !BleLink.badgeSupported) {
            status = "圆屏未连接或固件不支持电子吧唧"
            return
        }
        if (NaviBridge.guidanceActive) {
            status = "导航或巡航中不可上传图片，请停车结束导航后重试"
            return
        }
        synchronized(this) {
            if (encoding || pendingId >= 0) {
                status = "上一张图片仍在处理或等待设备确认"
                return
            }
            encoding = true
        }
        status = "正在处理图片…"
        Thread {
            try {
                val packet = BadgeActivity.encode(photo)
                handler.post {
                    encoding = false
                    if (!BleLink.isConnected || !BleLink.badgeSupported) {
                        status = "连接中断，尚未上传"
                    } else {
                        seq = (seq + 1) and 255
                        val id = seq
                        pendingId = id
                        if (BleLink.sendBulk(6, id, packet)) {
                            status = "BLE 传输中（约 ${packet.size / 1024} KB），等待圆屏确认…"
                            handler.postDelayed({
                                if (pendingId == id) {
                                    pendingId = -1
                                    status = "等待圆屏确认超时，可能未上传成功，请重试"
                                }
                            }, 120_000)
                        } else {
                            pendingId = -1
                            status = "蓝牙队列未就绪，无法开始上传"
                        }
                    }
                }
            } catch (_: Exception) {
                handler.post {
                    encoding = false
                    status = "处理图片失败，请换一张照片"
                }
            }
        }.start()
    }

    fun onAck(type: Int, id: Int, saved: Boolean?) {
        if (type != 6 || id != pendingId) return
        if (saved == null) {
            status = "图片已发送，等待存储状态…"
        } else {
            onStored(id, saved)
        }
    }

    fun onStored(id: Int, saved: Boolean) {
        if (id != pendingId) return // old or reordered notifications
        pendingId = -1
        status = if (saved) "上传成功，图片已保存到设备，可脱离手机显示"
                 else "图片已显示但未持久保存，断电后可能丢失"
    }

    fun connectionLost() {
        if (encoding || pendingId >= 0) {
            pendingId = -1
            encoding = false
            status = "蓝牙断开，图片上传未确认；重连后请重试"
        }
    }
}
