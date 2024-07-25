/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_wn_models.h"

#include "app/app_led.h"
#include "app/app_wifi.h"
#include "app/app_camera.h"
#include "app/app_lvgl.h"
#include "app/app_glm.h"
#include "utils/utils.h"
#include "utils/speech_wakeup.h"
#include "utils/file_manager.h"

static const char *TAG = "[MAIN]";

void nvs_initialise(void) {
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);
}

void device_info_print(void) {
    /* Print chip information */
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    printf("This is %s chip with %d CPU core(s), WiFi%s%s, ",
            CONFIG_IDF_TARGET,
            chip_info.cores,
            (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
            (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

    printf("silicon revision %d, ", chip_info.revision);

    printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
            (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    printf("Minimum free heap size: %d bytes\n", esp_get_minimum_free_heap_size());
}

void SPIFFS_Directory(char *path)
{
	DIR *dir = opendir(path);
	assert(dir != NULL);
	while (true)
	{
		struct dirent *pe = readdir(dir);
		if (!pe)
			break;
		ESP_LOGI(__FUNCTION__, "d_name=%s d_ino=%d d_type=%x", pe->d_name, pe->d_ino, pe->d_type);
	}
	closedir(dir);
}

void mount_spiffs()
{
    ESP_LOGI(TAG, "Initializing SPIFFS");
	esp_vfs_spiffs_conf_t conf = {
		.base_path = "/spiffs",
		.partition_label = "storage",
		.max_files = 20,
		.format_if_mount_failed = false};
	esp_err_t ret = esp_vfs_spiffs_register(&conf);
	if (ret != ESP_OK)
	{
		if (ret == ESP_FAIL)
			ESP_LOGE(TAG, "Failed to mount or format filesystem");
		else if (ret == ESP_ERR_NOT_FOUND)
			ESP_LOGE(TAG, "Failed to find SPIFFS partition");
		else
			ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
		return;
	}
	/*显示spiffs里的文件列表*/
	SPIFFS_Directory("/spiffs/");
}

void app_main(void)
{
    printf("Hello world!\n");

    // print device info
    device_info_print();

    // initial nvs flash
    nvs_initialise();

    // mount spiffs
    // mount_spiffs();

    // initialize wifi
    app_wifi_initialise();

    // initialize gui
    // app_lvgl_initialise();

    // initialize camera
    // app_camera_initialise();

    // wait for wifi connection
    // app_wifi_wait_connected();

    // init nstp time
    // sync_with_nstp();

    // initialize glm
    // app_glm_initialise();

    // create led task
    // xTaskCreate(led_task, "led_task", 2048, NULL, 5, NULL);

    // create camera task
    // xTaskCreate(camera_task, "camera_task", 4096*2, NULL, 5, NULL);

    // lvgl task
    // xTaskCreate(&app_lvgl_initialise, "lvgl_task", 4096*2, NULL, 0, NULL);

    // init wakenet
    speech_wakeup_initialise();
}
