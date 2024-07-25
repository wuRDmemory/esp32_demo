/*
 * @Descripttion :
 * @version      :
 * @Author       : Kevincoooool
 * @Date         : 2021-05-25 16:03:28
 * @LastEditors: Please set LastEditors
 * @LastEditTime: 2022-07-07 18:06:13
 * @FilePath: \S3_DEMO\8.speech_rec\main\app_speech_recsrc.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "xtensa/core-macros.h"
#include "esp_partition.h"
#include "speech_srcif.h"
#include "driver/i2s.h"
#include "esp_log.h"
#include "esp_spiffs.h"
// #include "esp_ns.h"
// #include "esp_agc.h"

#define I2S_NUM 1
// extern bool playing;

void record_i2s_initialise(void) {
    i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_RX,        // the mode must be set according to DSP configuration
        .sample_rate = 16000,                         // must be the same as DSP configuration
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT, // must be the same as DSP configuration
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,                        // must be the same as DSP configuration
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .dma_buf_count = 6,
        .dma_buf_len = 160,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0,
        .intr_alloc_flags = ESP_INTR_FLAG_LOWMED,
        .bits_per_chan = I2S_BITS_PER_SAMPLE_16BIT};
    i2s_pin_config_t pin_config = {
        .mck_io_num = -1,
        .bck_io_num = IIS_SCLK, // IIS_SCLK
        .ws_io_num = IIS_LCLK,  // IIS_LCLK
        .data_out_num = -1,     // IIS_DSIN
        .data_in_num = IIS_DOUT // IIS_DOUT
    };
    i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM, &pin_config);
    i2s_zero_dma_buffer(I2S_NUM);
}

static void tips_i2s_read(void) {
    static uint8_t i = 0;
    i++;
    if (i > 50) {
        i = 0;
        printf("i2s_reading...\r\n");
    }
}

static void tips_sleep(void) {
    static uint8_t i = 0;
    i++;
    if (i > 20) {
        i = 0;
        printf("sleeping...\r\n");
    }
}

/*
    语音识别中的录音任务
*/
void recsrc_task(void *arg) {
    // play_i2s_init();
    record_i2s_initialise();//初始化I2S用于录音

    src_cfg_t *cfg = (src_cfg_t *)arg;
    size_t samp_len = cfg->item_size * 2 * sizeof(int) / sizeof(int16_t);
    int *samp = malloc(samp_len);
    size_t read_len = 0;

    while (1) {
        // if (playing != true) {
            // tips_i2s_read();
            i2s_read(I2S_NUM, samp, samp_len, &read_len, portMAX_DELAY);//从I2S读取原始数据
            for (int x = 0; x < cfg->item_size / 4; x++)//对原始数据进行处理只取其中一部分 
            {
                int s1 = ((samp[x * 4] + samp[x * 4 + 1]) >> 13) & 0x0000FFFF;
                int s2 = ((samp[x * 4 + 2] + samp[x * 4 + 3]) << 3) & 0xFFFF0000;
                samp[x] = s1 | s2;
            }
            // esp_agc_process(agc_inst, samp, agc_out,samp_len, 16000);
            xQueueSend(*cfg->queue, samp, portMAX_DELAY);//把处理完成的数据发送到识别任务的队列
        // } else {
        //     // tips_sleep();
        //     memset(samp, 0, samp_len);
        //     vTaskDelay(10);
        // }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
}
