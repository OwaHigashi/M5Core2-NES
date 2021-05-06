// SPDX: MIT
// Copyright 2021 Brian Starkey <stark3y@gmail.com>
// Portions from lvgl example: https://github.com/lvgl/lv_port_esp32/blob/master/main/main.c
//
// 

#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include <esp_log.h>

#include "i2c_manager.h"
#include "m5core2_axp192.h"

#include "lvgl.h"
#include "lvgl_helpers.h"

#define LV_TICK_PERIOD_MS 1


enum toggle_id {
	TOGGLE_LED = 0,
	TOGGLE_VIB,
	TOGGLE_5V,
};

static void toggle_event_cb(lv_obj_t *toggle, lv_event_t event)
{
	if(event == LV_EVENT_VALUE_CHANGED) {
		bool state = lv_switch_get_state(toggle);
		enum toggle_id *id = lv_obj_get_user_data(toggle);

		// Note: This is running in the GUI thread, so prolonged i2c
		// comms might cause some jank
		switch (*id) {
		case TOGGLE_LED:
			m5core2_led(state);
			break;
		case TOGGLE_VIB:
			m5core2_vibration(state);
			break;
		case TOGGLE_5V:
			m5core2_int_5v(state);
			break;
		}
	}
}

static void gui_timer_tick(void *arg)
{
	// Unused
	(void) arg;

	lv_tick_inc(LV_TICK_PERIOD_MS);
}

static void gui_thread(void *pvParameter)
{
	(void) pvParameter;

	static lv_color_t bufs[2][DISP_BUF_SIZE];
	static lv_disp_buf_t disp_buf;
	uint32_t size_in_px = DISP_BUF_SIZE;

	// Set up the frame buffers
	lv_disp_buf_init(&disp_buf, &bufs[0], &bufs[1], size_in_px);

	// Set up the display driver
	lv_disp_drv_t disp_drv;
	lv_disp_drv_init(&disp_drv);
	disp_drv.flush_cb = disp_driver_flush;
	disp_drv.buffer = &disp_buf;
	lv_disp_drv_register(&disp_drv);

	// Register the touch screen. All of the properties of it
	// are set via the build config
	lv_indev_drv_t indev_drv;
	lv_indev_drv_init(&indev_drv);
	indev_drv.read_cb = touch_driver_read;
	indev_drv.type = LV_INDEV_TYPE_POINTER;
	lv_indev_drv_register(&indev_drv);

	// Timer to drive the main lvgl tick
	const esp_timer_create_args_t periodic_timer_args = {
		.callback = &gui_timer_tick,
		.name = "periodic_gui"
	};
	esp_timer_handle_t periodic_timer;
	ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
	ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, LV_TICK_PERIOD_MS * 1000));


	// Full screen root container
	lv_obj_t *root = lv_cont_create(lv_scr_act(), NULL);
	lv_obj_set_size(root, 320, 240);
	lv_cont_set_layout(root, LV_LAYOUT_COLUMN_MID);
	// Don't let the containers be clicked on
	lv_obj_set_click(root, false);

	// Create rows of switches for different functions
	struct {
		const char *label;
		bool init;
		enum toggle_id id;
	} switches[] = {
		{ "LED",     true,  TOGGLE_LED },
		{ "Vibrate", false, TOGGLE_VIB },
		{ "5V Bus",  false, TOGGLE_5V },
	};
	for (int i = 0; i < sizeof(switches) / sizeof(switches[0]); i++) {
		lv_obj_t *row = lv_cont_create(root, NULL);
		lv_cont_set_layout(row, LV_LAYOUT_ROW_MID);
		lv_obj_set_size(row, 200, 0);
		lv_cont_set_fit2(row, LV_FIT_NONE, LV_FIT_TIGHT);
		// Don't let the containers be clicked on
		lv_obj_set_click(row, false);

		lv_obj_t *toggle = lv_switch_create(row, NULL);
		if (switches[i].init) {
			lv_switch_on(toggle, LV_ANIM_OFF);
		}
		lv_obj_set_user_data(toggle, &switches[i].id);
		lv_obj_set_event_cb(toggle, toggle_event_cb);

		lv_obj_t *label = lv_label_create(row, NULL);
		lv_label_set_text(label, switches[i].label);
	}

	while (1) {
		vTaskDelay(10 / portTICK_PERIOD_MS);

		lv_task_handler();
	}

	// Never returns
}


void app_main(void)
{
	printf("Hello world!\n");

	/* Print chip information */
	esp_chip_info_t chip_info;
	esp_chip_info(&chip_info);
	printf("This is %s chip with %d CPU cores, WiFi%s%s, ",
			CONFIG_IDF_TARGET,
			chip_info.cores,
			(chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
			(chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

	printf("silicon revision %d, ", chip_info.revision);

	printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
			(chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

	printf("Free heap: %d\n", esp_get_free_heap_size());
	
	m5core2_init();

	lvgl_i2c_locking(i2c_manager_locking());

	lv_init();
	lvgl_driver_init();

	// Needs to be pinned to a core
	xTaskCreatePinnedToCore(gui_thread, "gui", 4096*2, NULL, 0, NULL, 1);

	printf("Running...\n");
	fflush(stdout);

	for ( ; ; ) {
		vTaskDelay(portMAX_DELAY);
	}
	printf("Restarting now.\n");
	esp_restart();
}
