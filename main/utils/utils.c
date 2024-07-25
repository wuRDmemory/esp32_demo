#include "utils.h"
#include "esp_log.h"
#include "esp_sntp.h"

static const char *TAG = "[UTILS]";

double get_unix_timestamp() {
    time(NULL);
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec / 1000.0 / 1000.0;
}

void sync_with_nstp() {
    esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();

    while (time(NULL) < 1000) {
        ESP_LOGI(TAG, "Waiting for system time to be set... ");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}