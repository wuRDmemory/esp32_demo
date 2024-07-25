/*
 * @Descripttion :
 * @version      :
 * @Author       : Kevincoooool
 * @Date         : 2021-05-25 16:03:28
 * @LastEditors: Please set LastEditors
 * @LastEditTime: 2022-03-09 14:27:45
 * @FilePath: \S3_DEMO\8.speech_rec\main\app_speech_wakeup.c
 */
#include "dl_lib_coefgetter_if.h"
#include "esp_partition.h"
#include "esp_spi_flash.h"
#include "esp_system.h"
#include "esp_mn_iface.h"
#include "esp_mn_models.h"
#include "esp_wn_iface.h"
#include "esp_wn_models.h"
#include "esp_afe_sr_models.h"
#include "esp_process_sdkconfig.h"
#include "model_path.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "lvgl/lvgl.h"
#include "lvgl_helpers.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "driver/i2s.h"
#include "speech_wakeup.h"
#include "xtensa/core-macros.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
// #include "esp_board_init.h"
#include "../hardware/bsp_i2s.h"
#include "../utils/file_manager.h"
#include "../utils/play_audio.h"

#define TAG "SPEECH"
#define ADC_I2S_CHANNEL 2
#define I2S_NUM 1

static esp_afe_sr_iface_t *afe_handle = NULL;
static esp_afe_sr_data_t *afe_data = NULL;
static srmodel_list_t *models = NULL;
static bool wake_flag = false;
static volatile int task_flag = 0;

void feed_Task(void *arg) {
    esp_afe_sr_data_t *afe_data = arg;
    int audio_chunksize = afe_handle->get_feed_chunksize(afe_data);
    int nch = afe_handle->get_channel_num(afe_data);
	size_t samp_len = audio_chunksize * sizeof(int) * ADC_I2S_CHANNEL;
    int *samp = (int *)malloc(samp_len); // int is 4 bytes
    assert(samp);

	size_t read_len = 0;
    while (1) {
        if (!get_playing_flag()) {
            i2s_read(1, samp, samp_len, &read_len, portMAX_DELAY);//从i2s读取原始数据
            // printf("audio_chunksize = %d read len: %d\n", audio_chunksize, samp_len);
            for (int x = 0; x < audio_chunksize; x++) {
                int s1 = ((samp[x * 4] + samp[x * 4 + 1]) >> 13) & 0x0000FFFF;
                int s2 = ((samp[x * 4 + 2] + samp[x * 4 + 3]) << 3) & 0xFFFF0000;
                samp[x] = s1 | s2;
            }
            // esp_get_feed_data(true, (int16_t*)samp, samp_len);
            afe_handle->feed(afe_data, samp);
        }
		vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    if (samp) {
        free(samp);
        samp = NULL;
    }
    vTaskDelete(NULL);
}

void detect_Task(void *arg) {
    esp_afe_sr_data_t *afe_data = arg;
    int afe_chunksize = afe_handle->get_fetch_chunksize(afe_data);
	int nch = afe_handle->get_channel_num(afe_data);
    int16_t *buff = (int16_t *)malloc(afe_chunksize * sizeof(int16_t));
    assert(buff);

    // build multi model
    char* mn_name = esp_srmodel_filter(models, ESP_MN_PREFIX, ESP_MN_CHINESE);
    ESP_LOGI(TAG, "MultiNet mn_name: %s\n", mn_name);
    esp_mn_iface_t *multinet = esp_mn_handle_from_name(mn_name);
    model_iface_data_t *model_data = multinet->create(mn_name, 6000);
    esp_mn_commands_update_from_sdkconfig(multinet, model_data);
    int mn_chunksize = multinet->get_samp_chunksize(model_data);
    assert(mn_chunksize == afe_chunksize);

    multinet->print_active_speech_commands(model_data);
    ESP_LOGI(TAG, "------------detect start------------\n");

    while (task_flag) {
        afe_fetch_result_t* res = afe_handle->fetch(afe_data); 
        if (!wake_flag) {
            if (!res || res->ret_value == ESP_FAIL) {
                ESP_LOGI(TAG, "fetch error!");
                break;
            }

            if (res->wakeup_state == WAKENET_DETECTED) {
                ESP_LOGI(TAG, "wakeword detected");
                ESP_LOGI(TAG, "model index:%d, word index:%d", res->wakenet_model_index, res->wake_word_index);
                ESP_LOGI(TAG, "-----------LISTENING-----------\n");
                play_sdfile_name("wav", "wozai.wav");
                afe_handle->disable_wakenet(afe_data);
                multinet->clean(model_data);
                wake_flag = true;
            }
        } else {
            esp_mn_state_t state = multinet->detect(model_data, res->data);
            if (state == ESP_MN_STATE_DETECTING) {
                continue;
            }

            if (state == ESP_MN_STATE_DETECTED) {
                ESP_LOGI(TAG, "command detected");
                esp_mn_results_t *mn_result = multinet->get_results(model_data);
                int command_id = -1;
                float prob = 0;
                for (int i = 0; i < mn_result->num; i++) {
                    ESP_LOGI(TAG, "command: %d, score: %.3f, string: %s", mn_result->command_id[i], mn_result->prob[i], mn_result->string);
                }
                play_sdfile_name("wav", "haode.wav");
                ESP_LOGI(TAG, "-----------listening-----------\n");
            } else if (state == ESP_MN_STATE_TIMEOUT) {
                ESP_LOGI(TAG, "multi timeout");
                wake_flag = false;
                afe_handle->enable_wakenet(afe_data);
                ESP_LOGI(TAG, "------------detect start------------\n");
            }
        }

		vTaskDelay(10 / portTICK_PERIOD_MS);
    }

	ESP_LOGI(TAG, "detect_Task exit");
    if (buff) {
        free(buff);
        buff = NULL;
    }
    vTaskDelete(NULL);
}

void speech_wakeup_initialise() {
    // init
    record_i2s_initialise();
    // play_i2s_init();
    // if (ESP_OK != esp_board_init(16000, 2, 16)) {
    //     ESP_LOGE(TAG, "board init failed");
    //     return;
    // }
	
    // build model
    // esp_sdcard_init("/sdcard", 1);
    fm_sdcard_init(MOUNT_POINT);
    models = esp_srmodel_init(MOUNT_POINT);
    ESP_LOGI(TAG, "model num: %d\n", models->num);
    char *wn_name = esp_srmodel_filter(models, ESP_WN_PREFIX, "jarvis"); // select WakeNet with "alexa" wake word.;
    char *wn_name_2 = NULL;
    ESP_LOGI(TAG, "WakeNet wn_name: %s\n", wn_name);

    //
    // step3: initialize model
    //
    afe_handle = (esp_afe_sr_iface_t *)&ESP_AFE_SR_HANDLE;
    afe_config_t afe_config = AFE_CONFIG_DEFAULT();
    afe_config.memory_alloc_mode = AFE_MEMORY_ALLOC_MORE_PSRAM;
	afe_config.aec_init = false;
	afe_config.se_init = true;
	afe_config.vad_init = true;
    afe_config.wakenet_init = true;
	afe_config.wakenet_mode = DET_MODE_95;
    afe_config.wakenet_model_name = wn_name;
    // afe_config.wakenet_model_name_2 = wn_name_2;
    afe_config.voice_communication_init = false;
	afe_config.pcm_config.mic_num = 1;
	afe_config.pcm_config.ref_num = 0; // TODO
	afe_config.pcm_config.total_ch_num = 1;
	afe_config.pcm_config.sample_rate = 16000;
    afe_config.afe_ringbuf_size = 150;
    afe_data = afe_handle->create_from_config(&afe_config);

	task_flag = 1;
    xTaskCreatePinnedToCore(&feed_Task, "feed", 8 * 1024, (void*)afe_data, 5, NULL, 0);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    xTaskCreatePinnedToCore(&detect_Task, "detect", 4 * 1024, (void*)afe_data, 5, NULL, 1);
}
