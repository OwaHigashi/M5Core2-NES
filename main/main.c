#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_partition.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "i2c_manager.h"
#include "m5core2_axp192.h"
#include "nofrendo.h"

void app_main(void)
{
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

	printf("NoFrendo start!\n");
	nofrendo_main(0, NULL);

}

char *osd_getromdata() {
	char* romdata;
	const esp_partition_t* part;
	spi_flash_mmap_handle_t hrom;
	esp_err_t err;
	nvs_flash_init();
	part=esp_partition_find_first(0x40, 1, NULL);
	if (part==0) printf("Couldn't find rom part!\n");
	err=esp_partition_mmap(part, 0, 3*1024*1024, SPI_FLASH_MMAP_DATA, (const void**)&romdata, &hrom);
	if (err!=ESP_OK) printf("Couldn't map rom part!\n");
	printf("Initialized. ROM@%p\n", romdata);
    return (char*)romdata;
}
