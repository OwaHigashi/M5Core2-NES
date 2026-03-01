// M5Core2 bridge layer - C++ wrappers for nofrendo C code
#include <M5Core2.h>
#include <Wire.h>
#include <SD.h>
#include <esp_heap_caps.h>

extern "C" {
    // Called by main (replaces m5core2_axp192 submodule)
    void m5core2_init(void);
    esp_err_t m5core2_speaker(bool on);

    // Called by psxcontroller.c (replaces lvgl_esp32_drivers FT6X06)
    void ft6x06_init(uint16_t dev_addr);
    bool ft6x36_direct_read(int16_t* x1, int16_t* y1, int16_t* x2, int16_t* y2, uint8_t* count);

    // Called by spi_lcd.c (M5.Lcd wrappers)
    void m5lcd_init_for_nes(void);
    void m5lcd_start_write(void);
    void m5lcd_end_write(void);
    void m5lcd_set_addr_window(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
    void m5lcd_write_pixels(uint16_t *data, uint32_t len);

    // Called by nes_rom.c (ROM loading from SD card)
    char *osd_getromdata(void);

    // Called from setup() before nofrendo_main()
    void select_rom_from_sd(void);
}

// ---- AXP192 / Power ----

void m5core2_init(void) {
    // Already initialized by M5.begin() in setup()
}

esp_err_t m5core2_speaker(bool on) {
    M5.Axp.SetSpkEnable(on);
    return ESP_OK;
}

// ---- Touch (FT6336U via I2C) ----

void ft6x06_init(uint16_t dev_addr) {
    // Already initialized by M5.begin()
}

bool ft6x36_direct_read(int16_t* x1, int16_t* y1, int16_t* x2, int16_t* y2, uint8_t* count) {
    uint8_t buf[11];

    Wire1.beginTransmission(0x38);
    Wire1.write(0x02);  // TD_STATUS register
    if (Wire1.endTransmission(false) != 0) {
        *count = 0;
        return false;
    }

    if (Wire1.requestFrom((uint8_t)0x38, (uint8_t)11) != 11) {
        *count = 0;
        return false;
    }
    for (int i = 0; i < 11; i++) {
        buf[i] = Wire1.read();
    }

    *count = buf[0] & 0x0F;
    if (*count > 2) *count = 0;

    // Register layout from 0x02:
    // [0] TD_STATUS (touch count)
    // [1] P1_XH (event + X high), [2] P1_XL (X low)
    // [3] P1_YH (ID + Y high),    [4] P1_YL (Y low)
    // [5] P1_WEIGHT, [6] P1_MISC
    // [7] P2_XH, [8] P2_XL
    // [9] P2_YH, [10] P2_YL

    if (*count > 0) {
        *x1 = ((buf[1] & 0x0F) << 8) | buf[2];
        *y1 = ((buf[3] & 0x0F) << 8) | buf[4];
    }
    if (*count > 1) {
        *x2 = ((buf[7] & 0x0F) << 8) | buf[8];
        *y2 = ((buf[9] & 0x0F) << 8) | buf[10];
    }

    return true;
}

// ---- LCD wrappers ----

void m5lcd_init_for_nes(void) {
    M5.Lcd.setSwapBytes(true);
    M5.Lcd.fillScreen(BLACK);
}

void m5lcd_start_write(void) {
    M5.Lcd.startWrite();
}

void m5lcd_end_write(void) {
    M5.Lcd.endWrite();
}

void m5lcd_set_addr_window(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    M5.Lcd.setAddrWindow(x, y, w, h);
}

void m5lcd_write_pixels(uint16_t *data, uint32_t len) {
    M5.Lcd.pushColors(data, len, true);
}

// ---- ROM selection UI and loading from SD card ----

#define MAX_ROMS 500
#define ROM_NAME_LEN 128
#define ITEMS_PER_PAGE 6
#define ITEM_HEIGHT 30
#define LIST_Y_START 35
#define TITLE_HEIGHT 30

static uint8_t *rom_data = NULL;
static char selected_rom_path[256] = "";

static void show_error(const char *msg) {
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextColor(RED);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(20, 80);
    M5.Lcd.println("NES ROM not found!");
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(20, 120);
    M5.Lcd.println(msg);
    M5.Lcd.setCursor(20, 140);
    M5.Lcd.println("Put .nes files in /nes/");
    M5.Lcd.setCursor(20, 155);
    M5.Lcd.println("folder on SD card.");
    while (1) { vTaskDelay(1000 / portTICK_PERIOD_MS); }
}

static void draw_rom_list(char **names, int count, int selected, int scroll) {
    // Title bar with page info
    M5.Lcd.fillRect(0, 0, 320, TITLE_HEIGHT, NAVY);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(10, 7);
    M5.Lcd.print("ROM Select");
    // Page indicator (e.g., "3/84")
    int cur_page = scroll / ITEMS_PER_PAGE + 1;
    int total_pages = (count + ITEMS_PER_PAGE - 1) / ITEMS_PER_PAGE;
    char page_str[16];
    snprintf(page_str, sizeof(page_str), "%d/%d", cur_page, total_pages);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(260, 11);
    M5.Lcd.setTextColor(CYAN);
    M5.Lcd.print(page_str);

    // List area
    M5.Lcd.fillRect(0, LIST_Y_START, 320, ITEMS_PER_PAGE * ITEM_HEIGHT, BLACK);

    for (int i = 0; i < ITEMS_PER_PAGE && (scroll + i) < count; i++) {
        int idx = scroll + i;
        int y = LIST_Y_START + i * ITEM_HEIGHT;

        if (idx == selected) {
            M5.Lcd.fillRect(0, y, 320, ITEM_HEIGHT - 1, 0x1082);  // dark blue highlight
            M5.Lcd.setTextColor(YELLOW);
        } else {
            M5.Lcd.setTextColor(WHITE);
        }

        M5.Lcd.setTextSize(2);
        M5.Lcd.setCursor(10, y + 6);

        // Truncate long names to fit screen
        char display_name[24];
        strncpy(display_name, names[idx], 23);
        display_name[23] = '\0';
        // Remove .nes extension for cleaner display
        int len = strlen(display_name);
        if (len > 4 && (strcmp(display_name + len - 4, ".nes") == 0 ||
                        strcmp(display_name + len - 4, ".NES") == 0)) {
            display_name[len - 4] = '\0';
        }

        M5.Lcd.print(idx == selected ? "> " : "  ");
        M5.Lcd.print(display_name);
    }

    // Scroll indicators
    M5.Lcd.setTextColor(DARKGREY);
    M5.Lcd.setTextSize(1);
    if (scroll > 0) {
        M5.Lcd.setCursor(155, LIST_Y_START - 8);
        M5.Lcd.print("^");
    }
    if (scroll + ITEMS_PER_PAGE < count) {
        M5.Lcd.setCursor(155, LIST_Y_START + ITEMS_PER_PAGE * ITEM_HEIGHT + 2);
        M5.Lcd.print("v");
    }

    // Bottom bar: [<<PREV] [SELECT] [NEXT>>]
    int bottom_y = 220;
    M5.Lcd.fillRect(0, bottom_y, 320, 20, 0x2104);
    M5.Lcd.setTextColor(LIGHTGREY);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(5, bottom_y + 6);
    M5.Lcd.print("[<<PREV]");
    M5.Lcd.setCursor(126, bottom_y + 6);
    M5.Lcd.print("[SELECT]");
    M5.Lcd.setCursor(255, bottom_y + 6);
    M5.Lcd.print("[NEXT>>]");
}

// Called from setup() before nofrendo_main()
void select_rom_from_sd(void) {
    // Allocate ROM name list from PSRAM
    char **rom_names = (char **)heap_caps_malloc(MAX_ROMS * sizeof(char *), MALLOC_CAP_SPIRAM);
    if (!rom_names) {
        show_error("Memory allocation failed");
        return;
    }
    int rom_count = 0;

    File dir = SD.open("/nes");
    if (!dir) {
        free(rom_names);
        show_error("No /nes directory on SD card");
        return;
    }

    File entry;
    while ((entry = dir.openNextFile()) && rom_count < MAX_ROMS) {
        String name = entry.name();
        if (!entry.isDirectory() && (name.endsWith(".nes") || name.endsWith(".NES"))) {
            const char *fname = name.c_str();
            const char *slash = strrchr(fname, '/');
            if (slash) fname = slash + 1;
            rom_names[rom_count] = (char *)heap_caps_malloc(ROM_NAME_LEN, MALLOC_CAP_SPIRAM);
            if (rom_names[rom_count]) {
                strncpy(rom_names[rom_count], fname, ROM_NAME_LEN - 1);
                rom_names[rom_count][ROM_NAME_LEN - 1] = '\0';
                rom_count++;
            }
        }
        entry.close();
    }
    dir.close();
    printf("Found %d ROMs\n", rom_count);

    if (rom_count == 0) {
        free(rom_names);
        show_error("No .nes file found in /nes/");
        return;
    }

    // Auto-select if only one ROM
    if (rom_count == 1) {
        snprintf(selected_rom_path, sizeof(selected_rom_path), "/nes/%s", rom_names[0]);
        free(rom_names[0]);
        free(rom_names);
        return;
    }

    // Show selection UI
    int selected = 0;
    int scroll = 0;
    bool need_redraw = true;
    bool was_touched = false;

    while (1) {
        M5.update();

        if (need_redraw) {
            draw_rom_list(rom_names, rom_count, selected, scroll);
            need_redraw = false;
        }

        // Check touch
        if (M5.Touch.ispressed()) {
            if (!was_touched) {
                was_touched = true;
                TouchPoint_t p = M5.Touch.getPressPoint();

                if (p.y >= LIST_Y_START && p.y < LIST_Y_START + ITEMS_PER_PAGE * ITEM_HEIGHT) {
                    // Tapped on a list item
                    int tapped = scroll + (p.y - LIST_Y_START) / ITEM_HEIGHT;
                    if (tapped < rom_count) {
                        if (tapped == selected) {
                            // Double-tap on same item = confirm
                            break;
                        }
                        selected = tapped;
                        if (selected < scroll) scroll = selected;
                        if (selected >= scroll + ITEMS_PER_PAGE) scroll = selected - ITEMS_PER_PAGE + 1;
                        need_redraw = true;
                    }
                } else if (p.y >= 220) {
                    // Bottom bar: PREV PAGE / SELECT / NEXT PAGE
                    if (p.x < 107) {
                        // PREV PAGE
                        if (scroll > 0) {
                            scroll -= ITEMS_PER_PAGE;
                            if (scroll < 0) scroll = 0;
                            selected = scroll;
                            need_redraw = true;
                        }
                    } else if (p.x >= 213) {
                        // NEXT PAGE
                        if (scroll + ITEMS_PER_PAGE < rom_count) {
                            scroll += ITEMS_PER_PAGE;
                            if (scroll >= rom_count) scroll = rom_count - ITEMS_PER_PAGE;
                            if (scroll < 0) scroll = 0;
                            selected = scroll;
                            need_redraw = true;
                        }
                    } else {
                        // SELECT - confirm and exit
                        break;
                    }
                }
            }
        } else {
            was_touched = false;
        }

        vTaskDelay(50 / portTICK_PERIOD_MS);
    }

    snprintf(selected_rom_path, sizeof(selected_rom_path), "/nes/%s", rom_names[selected]);
    printf("Selected ROM: %s\n", selected_rom_path);

    // Free ROM name list
    for (int i = 0; i < rom_count; i++) free(rom_names[i]);
    free(rom_names);
}

char *osd_getromdata(void) {
    if (rom_data) return (char *)rom_data;

    if (selected_rom_path[0] == '\0') {
        show_error("No ROM selected");
        return NULL;
    }

    File f = SD.open(selected_rom_path);
    if (!f) {
        show_error("Failed to open ROM file");
        return NULL;
    }

    size_t rom_size = f.size();
    rom_data = (uint8_t *)heap_caps_malloc(rom_size, MALLOC_CAP_SPIRAM);
    if (rom_data) {
        f.read(rom_data, rom_size);
        printf("Loaded ROM: %s (%d bytes)\n", selected_rom_path, (int)rom_size);
    } else {
        printf("Failed to allocate %d bytes for ROM!\n", (int)rom_size);
    }
    f.close();

    if (!rom_data) {
        show_error("Memory allocation failed");
    }

    return (char *)rom_data;
}
