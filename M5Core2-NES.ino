//========================================================================
// M5Core2-NES - NES Emulator (nofrendo) for M5Core2
// Based on ESP-IDF nofrendo port, adapted for Arduino IDE
// ROM: Place .nes files in /nes/ directory on SD card
// Controls: Touch screen zones (see psxcontroller.c)
//========================================================================
#include <M5Core2.h>
#define SDU_ENABLE_GZ
#include <M5StackUpdater.h>

extern "C" {
    int nofrendo_main(int argc, char *argv[]);
    void select_rom_from_sd(void);
}

void setup() {
    M5.begin();
    checkSDUpdater(SD, MENU_BIN, 2000, TFCARD_CS_PIN);

    select_rom_from_sd();

    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(60, 100);
    M5.Lcd.println("NES Emulator");
    M5.Lcd.setCursor(50, 130);
    M5.Lcd.println("Loading ROM...");

    printf("NoFrendo start!\n");
    nofrendo_main(0, NULL);  // Blocking - never returns
}

void loop() {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}
