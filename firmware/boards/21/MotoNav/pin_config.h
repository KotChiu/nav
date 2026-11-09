#pragma once

// Waveshare ESP32-S3-Touch-LCD-2.1
// ST7701S 3-wire SPI control interface + 16-bit RGB refresh interface.

#define LCD_CLK 2
#define LCD_MOSI 1
#define LCD_BL 6
#define LCD_CS (-1)
#define LCD_RESET (-1)

#define LCD_HSYNC 38
#define LCD_VSYNC 39
#define LCD_DE 40
#define LCD_PCLK 41

#define LCD_DATA0 5
#define LCD_DATA1 45
#define LCD_DATA2 48
#define LCD_DATA3 47
#define LCD_DATA4 21
#define LCD_DATA5 14
#define LCD_DATA6 13
#define LCD_DATA7 12
#define LCD_DATA8 11
#define LCD_DATA9 10
#define LCD_DATA10 9
#define LCD_DATA11 46
#define LCD_DATA12 3
#define LCD_DATA13 8
#define LCD_DATA14 18
#define LCD_DATA15 17

// The existing renderer is authored around a 466x466 logical canvas.
#define LCD_WIDTH 466
#define LCD_HEIGHT 466
#define PANEL_WIDTH 480
#define PANEL_HEIGHT 480

#define IIC_SDA 15
#define IIC_SCL 7
#define TP_INT 16
#define TP_RESET (-1)

#define BOARD_TOUCH_FLIP_X 0
#define BOARD_TOUCH_FLIP_Y 0

// 3.7V battery voltage is measured through GPIO4 on this board.
#define BAT_ADC_PIN 4

// 1-bit SDMMC shares the LCD setup clock and command pins.
#define SDMMC_CLK 2
#define SDMMC_CMD 1
#define SDMMC_DATA 42
