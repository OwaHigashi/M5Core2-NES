// spi_lcd.c - M5Core2 LCD adapter for nofrendo NES emulator
// Replaces direct SPI register access with M5.Lcd API wrappers

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "spi_lcd.h"
#include <esp_heap_caps.h>

// M5.Lcd C wrappers (defined in m5_bridge.cpp)
extern void m5lcd_init_for_nes(void);
extern void m5lcd_start_write(void);
extern void m5lcd_end_write(void);
extern void m5lcd_set_addr_window(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
extern void m5lcd_write_pixels(uint16_t *data, uint32_t len);

extern uint16_t myPalette[];

static uint16_t *line_buf = NULL;

void ili9341_write_frame(const uint16_t xs, const uint16_t ys, const uint16_t width, const uint16_t height, const uint8_t * data[])
{
    int x, y;

    if (!line_buf) {
        line_buf = (uint16_t *)heap_caps_malloc(width * sizeof(uint16_t), MALLOC_CAP_DMA);
        if (!line_buf) {
            printf("Failed to allocate line buffer!\n");
            return;
        }
    }

    m5lcd_start_write();
    m5lcd_set_addr_window(xs, ys, width, height);

    for (y = 0; y < height; y++) {
        if (data == NULL) {
            memset(line_buf, 0, width * sizeof(uint16_t));
        } else {
            for (x = 0; x < width; x++) {
                line_buf[x] = myPalette[(unsigned char)(data[y][x])];
            }
        }
        m5lcd_write_pixels(line_buf, width);
    }

    m5lcd_end_write();
}

void ili9341_nes_init()
{
    printf("LCD init (using M5Core2 LCD)...\n");
    m5lcd_init_for_nes();
}
