#pragma once

#include <Arduino_GFX_Library.h>
#include <Wire.h>
#include <driver/ledc.h>
#include <driver/spi_master.h>
#include <esp_system.h>

#define TCA9554_ADDR 0x20
#define TCA9554_INPUT_REG 0x00
#define TCA9554_OUTPUT_REG 0x01
#define TCA9554_CONFIG_REG 0x03

#define TCA_LCD_RESET_BIT 0
#define TCA_TOUCH_RESET_BIT 1
#define TCA_LCD_CS_BIT 2

static bool tcaWrite(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(TCA9554_ADDR);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

static bool tcaSetBit(uint8_t bit, bool high) {
  Wire.beginTransmission(TCA9554_ADDR);
  Wire.write(TCA9554_OUTPUT_REG);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((uint8_t)TCA9554_ADDR, (uint8_t)1) != 1) return false;
  uint8_t value = Wire.read();
  uint8_t mask = 1 << bit;
  value = high ? (value | mask) : (value & ~mask);
  return tcaWrite(TCA9554_OUTPUT_REG, value);
}

static void board21LcdReset() {
  tcaSetBit(TCA_LCD_RESET_BIT, false);
  delay(10);
  tcaSetBit(TCA_LCD_RESET_BIT, true);
  delay(100);
}

static void board21TouchReset() {
  tcaSetBit(TCA_TOUCH_RESET_BIT, false);
  delay(30);
  tcaSetBit(TCA_TOUCH_RESET_BIT, true);
  delay(50);
}

static spi_device_handle_t board21Spi = nullptr;

static void st7701Command(uint8_t command) {
  spi_transaction_t transaction = {};
  transaction.cmd = 0;
  transaction.addr = command;
  spi_device_transmit(board21Spi, &transaction);
}

static void st7701Data(uint8_t data) {
  spi_transaction_t transaction = {};
  transaction.cmd = 1;
  transaction.addr = data;
  spi_device_transmit(board21Spi, &transaction);
}

struct St7701InitStep {
  uint8_t command;
  uint8_t data[16];
  uint8_t length;
  uint16_t delayMs;
};

static const St7701InitStep ST7701_INIT[] = {
  {0xFF, {0x77, 0x01, 0x00, 0x00, 0x10}, 5, 0},
  {0xC0, {0x3B, 0x00}, 2, 0},
  {0xC1, {0x0B, 0x02}, 2, 0},
  {0xC2, {0x07, 0x02}, 2, 0},
  {0xCC, {0x10}, 1, 0},
  {0xCD, {0x08}, 1, 0},
  {0xB0, {0x00, 0x11, 0x16, 0x0E, 0x11, 0x06, 0x05, 0x09, 0x08, 0x21, 0x06, 0x13, 0x10, 0x29, 0x31, 0x18}, 16, 0},
  {0xB1, {0x00, 0x11, 0x16, 0x0E, 0x11, 0x07, 0x05, 0x09, 0x09, 0x21, 0x05, 0x13, 0x11, 0x2A, 0x31, 0x18}, 16, 0},
  {0xFF, {0x77, 0x01, 0x00, 0x00, 0x11}, 5, 0},
  {0xB0, {0x6D}, 1, 0},
  {0xB1, {0x37}, 1, 0},
  {0xB2, {0x81}, 1, 0},
  {0xB3, {0x80}, 1, 0},
  {0xB5, {0x43}, 1, 0},
  {0xB7, {0x85}, 1, 0},
  {0xB8, {0x20}, 1, 0},
  {0xC1, {0x78}, 1, 0},
  {0xC2, {0x78}, 1, 0},
  {0xD0, {0x88}, 1, 0},
  {0xE0, {0x00, 0x00, 0x02}, 3, 0},
  {0xE1, {0x03, 0xA0, 0x00, 0x00, 0x04, 0xA0, 0x00, 0x00, 0x00, 0x20, 0x20}, 11, 0},
  {0xE2, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 13, 0},
  {0xE3, {0x00, 0x00, 0x11, 0x00}, 4, 0},
  {0xE4, {0x22, 0x00}, 2, 0},
  {0xE5, {0x05, 0xEC, 0xA0, 0xA0, 0x07, 0xEE, 0xA0, 0xA0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 16, 0},
  {0xE6, {0x00, 0x00, 0x11, 0x00}, 4, 0},
  {0xE7, {0x22, 0x00}, 2, 0},
  {0xE8, {0x06, 0xED, 0xA0, 0xA0, 0x08, 0xEF, 0xA0, 0xA0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 16, 0},
  {0xEB, {0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00}, 7, 0},
  {0xED, {0xFF, 0xFF, 0xFF, 0xBA, 0x0A, 0xBF, 0x45, 0xFF, 0xFF, 0x54, 0xFB, 0xA0, 0xAB, 0xFF, 0xFF, 0xFF}, 16, 0},
  {0xEF, {0x10, 0x0D, 0x04, 0x08, 0x3F, 0x1F}, 6, 0},
  {0xFF, {0x77, 0x01, 0x00, 0x00, 0x13}, 5, 0},
  {0xEF, {0x08}, 1, 0},
  {0xFF, {0x77, 0x01, 0x00, 0x00, 0x00}, 5, 0},
  {0x36, {0x00}, 1, 0},
  {0x3A, {0x66}, 1, 0},
  {0x11, {0x00}, 0, 480},
  {0x20, {0x00}, 0, 120},
  {0x29, {0x00}, 0, 0},
};

static void st7701Init() {
  spi_bus_config_t busConfig = {};
  busConfig.mosi_io_num = LCD_MOSI;
  busConfig.miso_io_num = -1;
  busConfig.sclk_io_num = LCD_CLK;
  busConfig.quadwp_io_num = -1;
  busConfig.quadhd_io_num = -1;
  busConfig.max_transfer_sz = 64;
  if (spi_bus_initialize(SPI2_HOST, &busConfig, SPI_DMA_CH_AUTO) != ESP_OK) return;

  spi_device_interface_config_t deviceConfig = {};
  deviceConfig.command_bits = 1;
  deviceConfig.address_bits = 8;
  deviceConfig.mode = 0;
  deviceConfig.clock_speed_hz = 4000000;
  deviceConfig.spics_io_num = -1;
  deviceConfig.queue_size = 1;
  if (spi_bus_add_device(SPI2_HOST, &deviceConfig, &board21Spi) != ESP_OK) return;

  tcaSetBit(TCA_LCD_CS_BIT, false);
  for (const auto &step : ST7701_INIT) {
    st7701Command(step.command);
    for (uint8_t i = 0; i < step.length; i++) st7701Data(step.data[i]);
    if (step.delayMs) delay(step.delayMs);
  }
  tcaSetBit(TCA_LCD_CS_BIT, true);
}

static void board21Backlight(uint8_t brightness) {
  static bool attached = false;
  if (!attached) {
    ledcAttach(LCD_BL, 20000, 10);
    attached = true;
  }
  uint32_t duty = map(brightness, 0, 255, 0, 1023);
  ledcWrite(LCD_BL, duty);
}

static void board21HardwareInit() {
  Wire.begin(IIC_SDA, IIC_SCL, 400000);
  tcaWrite(TCA9554_CONFIG_REG, 0x00);
  tcaWrite(TCA9554_OUTPUT_REG, 0x00);
  board21LcdReset();
  st7701Init();
  board21TouchReset();
  board21Backlight(120);
}

class Board21Panel : public Arduino_RGB_Display {
public:
  Board21Panel(Arduino_ESP32RGBPanel *rgbPanel)
      : Arduino_RGB_Display(PANEL_WIDTH, PANEL_HEIGHT, rgbPanel, 0, true,
                            nullptr, GFX_NOT_DEFINED, nullptr, 0) {}

  bool begin(int32_t speed = GFX_NOT_DEFINED) override {
    (void)speed;
    board21HardwareInit();
    return Arduino_RGB_Display::begin(GFX_SKIP_DATABUS_BEGIN);
  }

  void setBrightness(uint8_t brightness) {
    board21Backlight(brightness);
  }

  void displayOn() {
    board21Backlight(120);
  }

  void displayOff() {
    board21Backlight(0);
  }
};
