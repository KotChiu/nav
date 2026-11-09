// ESP_LCD_Panel implementation for esp32s3.

#include "Arduino_ESP32RGBPanel.h"
#include <esp_heap_caps.h>
#include <string.h>

//#if defined(ESP32) && (CONFIG_IDF_TARGET_ESP32S3)
#if defined(ESP32) && (CONFIG_IDF_TARGET_ESP32S3 || CONFIG_IDF_TARGET_ESP32P4)  //Modify
Arduino_ESP32RGBPanel::Arduino_ESP32RGBPanel(
    int8_t de, int8_t vsync, int8_t hsync, int8_t pclk,
    int8_t r0, int8_t r1, int8_t r2, int8_t r3, int8_t r4,
    int8_t g0, int8_t g1, int8_t g2, int8_t g3, int8_t g4, int8_t g5,
    int8_t b0, int8_t b1, int8_t b2, int8_t b3, int8_t b4,
    uint16_t hsync_polarity, uint16_t hsync_front_porch, uint16_t hsync_pulse_width, uint16_t hsync_back_porch,
    uint16_t vsync_polarity, uint16_t vsync_front_porch, uint16_t vsync_pulse_width, uint16_t vsync_back_porch,
    uint16_t pclk_active_neg, int32_t prefer_speed, bool useBigEndian,
    uint16_t de_idle_high, uint16_t pclk_idle_high, size_t bounce_buffer_size_px,
    bool frame_boundary_swap)
    : _de(de), _vsync(vsync), _hsync(hsync), _pclk(pclk),
      _r0(r0), _r1(r1), _r2(r2), _r3(r3), _r4(r4),
      _g0(g0), _g1(g1), _g2(g2), _g3(g3), _g4(g4), _g5(g5),
      _b0(b0), _b1(b1), _b2(b2), _b3(b3), _b4(b4),
      _hsync_polarity(hsync_polarity), _hsync_front_porch(hsync_front_porch), _hsync_pulse_width(hsync_pulse_width), _hsync_back_porch(hsync_back_porch),
      _vsync_polarity(vsync_polarity), _vsync_front_porch(vsync_front_porch), _vsync_pulse_width(vsync_pulse_width), _vsync_back_porch(vsync_back_porch),
      _pclk_active_neg(pclk_active_neg), _prefer_speed(prefer_speed), _useBigEndian(useBigEndian),
      _de_idle_high(de_idle_high), _pclk_idle_high(pclk_idle_high), _bounce_buffer_size_px(bounce_buffer_size_px),
      _frame_boundary_swap(frame_boundary_swap)
{
}

bool Arduino_ESP32RGBPanel::begin(int32_t speed)
{
  if (speed == GFX_NOT_DEFINED)
  {
#ifdef CONFIG_SPIRAM_MODE_QUAD
    _speed = 6000000L;
#else
    _speed = 12000000L;
#endif
  }
  else
  {
    _speed = speed;
  }

  return true;
}

uint16_t *Arduino_ESP32RGBPanel::getFrameBuffer(int16_t w, int16_t h)
{
  esp_lcd_rgb_panel_config_t panel_config = {
#if (!defined(ESP_ARDUINO_VERSION_MAJOR)) || (ESP_ARDUINO_VERSION_MAJOR < 3)
      .clk_src = LCD_CLK_SRC_PLL160M,
#else
      .clk_src = LCD_CLK_SRC_DEFAULT,
#endif
      .timings = {
          .pclk_hz = (uint32_t)((_prefer_speed == GFX_NOT_DEFINED) ? _speed : _prefer_speed),
          .h_res = (uint32_t)w,
          .v_res = (uint32_t)h,
          .hsync_pulse_width = _hsync_pulse_width,
          .hsync_back_porch = _hsync_back_porch,
          .hsync_front_porch = _hsync_front_porch,
          .vsync_pulse_width = _vsync_pulse_width,
          .vsync_back_porch = _vsync_back_porch,
          .vsync_front_porch = _vsync_front_porch,
          .flags = {
              .hsync_idle_low = (uint32_t)((_hsync_polarity == 0) ? 1 : 0),
              .vsync_idle_low = (uint32_t)((_vsync_polarity == 0) ? 1 : 0),
              .de_idle_high = _de_idle_high,
              .pclk_active_neg = _pclk_active_neg,
              .pclk_idle_high = _pclk_idle_high,
          },
      },
      .data_width = 16, // RGB565 in parallel mode, thus 16 bits in width
#if (!defined(ESP_ARDUINO_VERSION_MAJOR)) || (ESP_ARDUINO_VERSION_MAJOR < 3)
#else
      .bits_per_pixel = 16,
      .num_fbs = 1,
      .bounce_buffer_size_px = 0,
#endif
      .sram_trans_align = 8,
      .psram_trans_align = 64,
      .hsync_gpio_num = _hsync,
      .vsync_gpio_num = _vsync,
      .de_gpio_num = _de,
      .pclk_gpio_num = _pclk,
      .disp_gpio_num = GPIO_NUM_NC, // -1
      #if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
      .data_gpio_nums = {0},
      #endif
      .flags = {
          .disp_active_low = true,
          #if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
          .refresh_on_demand = false,
          #endif
          .fb_in_psram = true, // allocate frame buffer from PSRAM
          #if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
          .double_fb = false,
          .no_fb = false,
          .bb_invalidate_cache = false,
          #endif
      },
  };

  if (_useBigEndian)
  {
    panel_config.data_gpio_nums[0] = _g3;
    panel_config.data_gpio_nums[1] = _g4;
    panel_config.data_gpio_nums[2] = _g5;
    panel_config.data_gpio_nums[3] = _r0;
    panel_config.data_gpio_nums[4] = _r1;
    panel_config.data_gpio_nums[5] = _r2;
    panel_config.data_gpio_nums[6] = _r3;
    panel_config.data_gpio_nums[7] = _r4;
    panel_config.data_gpio_nums[8] = _b0;
    panel_config.data_gpio_nums[9] = _b1;
    panel_config.data_gpio_nums[10] = _b2;
    panel_config.data_gpio_nums[11] = _b3;
    panel_config.data_gpio_nums[12] = _b4;
    panel_config.data_gpio_nums[13] = _g0;
    panel_config.data_gpio_nums[14] = _g1;
    panel_config.data_gpio_nums[15] = _g2;
  }
  else
  {
    panel_config.data_gpio_nums[0] = _b0;
    panel_config.data_gpio_nums[1] = _b1;
    panel_config.data_gpio_nums[2] = _b2;
    panel_config.data_gpio_nums[3] = _b3;
    panel_config.data_gpio_nums[4] = _b4;
    panel_config.data_gpio_nums[5] = _g0;
    panel_config.data_gpio_nums[6] = _g1;
    panel_config.data_gpio_nums[7] = _g2;
    panel_config.data_gpio_nums[8] = _g3;
    panel_config.data_gpio_nums[9] = _g4;
    panel_config.data_gpio_nums[10] = _g5;
    panel_config.data_gpio_nums[11] = _r0;
    panel_config.data_gpio_nums[12] = _r1;
    panel_config.data_gpio_nums[13] = _r2;
    panel_config.data_gpio_nums[14] = _r3;
    panel_config.data_gpio_nums[15] = _r4;
  }

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  const size_t pixels = (size_t)w * h;
  if (_frame_boundary_swap && _bounce_buffer_size_px && pixels &&
      pixels % (_bounce_buffer_size_px * 2) == 0)
  {
    for (auto &frame : _frame_buffers)
      frame = static_cast<uint16_t *>(heap_caps_calloc(pixels, sizeof(uint16_t), MALLOC_CAP_SPIRAM));
    _frame_available = xSemaphoreCreateBinary();
    if (_frame_buffers[0] && _frame_buffers[1] && _frame_available)
    {
      panel_config.num_fbs = 0;
      panel_config.flags.no_fb = true;
      panel_config.bounce_buffer_size_px = _bounce_buffer_size_px;
      if (esp_lcd_new_rgb_panel(&panel_config, &_panel_handle) == ESP_OK)
      {
        esp_lcd_rgb_panel_event_callbacks_t callbacks = {};
        callbacks.on_bounce_empty = _fillBounceBuffer;
        ESP_ERROR_CHECK(esp_lcd_rgb_panel_register_event_callbacks(_panel_handle, &callbacks, this));
        ESP_ERROR_CHECK(esp_lcd_panel_reset(_panel_handle));
        ESP_ERROR_CHECK(esp_lcd_panel_init(_panel_handle));
        return _frame_buffers[0];
      }
    }
    for (auto &frame : _frame_buffers)
    {
      heap_caps_free(frame);
      frame = nullptr;
    }
    if (_frame_available) vSemaphoreDelete(_frame_available);
    _frame_available = nullptr;
    panel_config.num_fbs = 1;
    panel_config.flags.no_fb = false;
    panel_config.bounce_buffer_size_px = 0;
  }
#endif
  ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&panel_config, &_panel_handle));

  ESP_ERROR_CHECK(esp_lcd_panel_reset(_panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_init(_panel_handle));

#if (!defined(ESP_ARDUINO_VERSION_MAJOR)) || (ESP_ARDUINO_VERSION_MAJOR < 3)
  esp_rgb_panel_t *_rgb_panel;
  _rgb_panel = __containerof(_panel_handle, esp_rgb_panel_t, base);

  return (uint16_t *)_rgb_panel->fb;
#else
  void *frame_buffer = nullptr;
  ESP_ERROR_CHECK(esp_lcd_rgb_panel_get_frame_buffer(_panel_handle, 1, &frame_buffer));
  _frame_buffers[0] = (uint16_t *)frame_buffer;

  _vsync_ready = xSemaphoreCreateBinary();
  if (_vsync_ready)
  {
    xSemaphoreGive(_vsync_ready);
    esp_lcd_rgb_panel_event_callbacks_t callbacks = {};
    callbacks.on_vsync = _vsyncCallback;
    ESP_ERROR_CHECK(esp_lcd_rgb_panel_register_event_callbacks(
        _panel_handle, &callbacks, this));
  }

  return _frame_buffers[0];
#endif
}

uint16_t *Arduino_ESP32RGBPanel::getFrameBuffer(uint8_t index)
{
  return index < 2 ? _frame_buffers[index] : nullptr;
}

bool Arduino_ESP32RGBPanel::drawFrameBuffer(uint16_t *frame_buffer,
                                            int16_t w, int16_t h)
{
  if (frameBoundarySwapActive())
  {
    const int index = frame_buffer == _frame_buffers[0] ? 0 :
                      frame_buffer == _frame_buffers[1] ? 1 : -1;
    portENTER_CRITICAL(&_frame_lock);
    const bool accepted = index >= 0 && index != _scanning_frame && _queued_frame < 0;
    if (accepted) _queued_frame = index;
    portEXIT_CRITICAL(&_frame_lock);
    return accepted;
  }
  return _panel_handle &&
         esp_lcd_panel_draw_bitmap(_panel_handle, 0, 0, w, h,
                                   frame_buffer) == ESP_OK;
}

uint16_t *Arduino_ESP32RGBPanel::acquireFrameBuffer(TickType_t timeout)
{
  if (!frameBoundarySwapActive()) return _frame_buffers[0];
  const TickType_t started = xTaskGetTickCount();
  for (;;)
  {
    portENTER_CRITICAL(&_frame_lock);
    const int available = _queued_frame < 0 ? 1 - _scanning_frame : -1;
    portEXIT_CRITICAL(&_frame_lock);
    if (available >= 0) return _frame_buffers[available];
    const TickType_t elapsed = xTaskGetTickCount() - started;
    if (elapsed >= timeout || xSemaphoreTake(_frame_available, timeout - elapsed) != pdTRUE)
      return nullptr;
  }
}

bool IRAM_ATTR Arduino_ESP32RGBPanel::_fillBounceBuffer(
    esp_lcd_panel_handle_t, void *buffer, int position_px, int bytes, void *user_ctx)
{
  auto *self = static_cast<Arduino_ESP32RGBPanel *>(user_ctx);
  bool swapped = false;
  if (position_px == 0)
  {
    portENTER_CRITICAL_ISR(&self->_frame_lock);
    if (self->_queued_frame >= 0)
    {
      self->_scanning_frame = self->_queued_frame;
      self->_queued_frame = -1;
      swapped = true;
    }
    portEXIT_CRITICAL_ISR(&self->_frame_lock);
  }
  // The selected source stays unchanged for every chunk of this frame.
  memcpy(buffer, self->_frame_buffers[self->_scanning_frame] + position_px, bytes);
  BaseType_t wake = pdFALSE;
  if (swapped) xSemaphoreGiveFromISR(self->_frame_available, &wake);
  return wake == pdTRUE;
}

bool Arduino_ESP32RGBPanel::refresh()
{
  return _panel_handle && esp_lcd_rgb_panel_refresh(_panel_handle) == ESP_OK;
}

bool Arduino_ESP32RGBPanel::waitVsync()
{
  return _vsync_ready && xSemaphoreTake(_vsync_ready, portMAX_DELAY) == pdTRUE;
}

bool Arduino_ESP32RGBPanel::_vsyncCallback(
    esp_lcd_panel_handle_t panel,
    const esp_lcd_rgb_panel_event_data_t *event_data,
    void *user_ctx)
{
  (void)panel;
  (void)event_data;
  Arduino_ESP32RGBPanel *self = static_cast<Arduino_ESP32RGBPanel *>(user_ctx);
  if (!self || !self->_vsync_ready) return pdFALSE;
  BaseType_t higher_priority_task_woken = pdFALSE;
  xSemaphoreGiveFromISR(self->_vsync_ready, &higher_priority_task_woken);
  return higher_priority_task_woken == pdTRUE;
}

#endif // #if defined(ESP32) && (CONFIG_IDF_TARGET_ESP32S3)
