// Copyright 2015-2020 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ctype.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "sdmmc_cmd.h"
#include "file_manager.h"

#ifdef CONFIG_IDF_TARGET_ESP32
#include "driver/sdmmc_host.h"
#endif

static const char *TAG = "file manager";

static const char *partition_label = "audio";

#define FLN_MAX 512

// Pin mapping when using SPI mode.
// With this mapping, SD card can be used both in SPI and 1-line SD mode.
// Note that a pull-up on CS line is required in SD mode.
#define PIN_NUM_MISO 0
#define PIN_NUM_MOSI 4
#define PIN_NUM_CLK  3
#define PIN_NUM_CS   8

bool flag_mount = false;
static sdmmc_card_t *card;

void sd_unmount(void)
{
    if (flag_mount == true)
    {
        esp_vfs_fat_sdcard_unmount(MOUNT_POINT, card);
        ESP_LOGI(TAG, "Card unmounted");
    }
    spi_bus_free(SPI3_HOST);
}

esp_err_t fm_sdcard_init(const char *sd_path)
{
    if (NULL != card) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret_val = ESP_OK;

    /**
     * @brief Options for mounting the filesystem.
     *   If format_if_mount_failed is set to true, SD card will be partitioned and
     *   formatted in case when mounting fails.
     *
     */
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    /**
     * @brief Use settings defined above to initialize SD card and mount FAT filesystem.
     *   Note: esp_vfs_fat_sdmmc/sdspi_mount is all-in-one convenience functions.
     *   Please check its source code and implement error recovery when developing
     *   production applications.
     *
     */
    sdmmc_host_t host =
        SDSPI_HOST_DEFAULT();
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        .max_transfer_sz = 4000,
    };
    host.slot = SPI3_HOST;
    ret_val = spi_bus_initialize(host.slot, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret_val != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize bus.");
        return ret_val;
    }

    /**
     * @brief This initializes the slot without card detect (CD) and write protect (WP) signals.
     *   Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
     *
     */
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = host.slot;
    /**
     * @brief Enable internal pullups on enabled pins. The internal pullups
     *   are insufficient however, please make sure 10k external pullups are
     *   connected on the bus. This is for debug / example purpose only.
     */

    /* get FAT filesystem on SD card registered in VFS. */
    ret_val = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &card);

    /* Check for SDMMC mount result. */
    if (ret_val != ESP_OK) {
        if (ret_val == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                     "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                     "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret_val));
        }
        return ret_val;
    }

    /* Card has been initialized, print its properties. */
    sdmmc_card_print_info(stdout, card);

    return ret_val;

}

esp_err_t fm_spiffs_init(void)
{
    esp_err_t ret;
    ESP_LOGI(TAG, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
        .base_path = MOUNT_POINT,
        .partition_label = partition_label,
        .max_files = 5,   // This decides the maximum number of files that can be created on the storage
        .format_if_mount_failed = false
    };

    ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ESP_FAIL;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(partition_label, &total, &used);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);

    return ESP_OK;
}

static void TraverseDir(char *direntName, int level, int indent)
{
    DIR *p_dir = NULL;
    struct dirent *p_dirent = NULL;

    p_dir = opendir(direntName);

    if (p_dir == NULL) {
        printf("opendir error\n");
        return;
    }

    while ((p_dirent = readdir(p_dir)) != NULL) {
        char *backupDirName = NULL;

        if (p_dirent->d_name[0] == '.') {
            continue;
        }

        int i;

        for (i = 0; i < indent; i++) {
            // printf("|");
            printf("     ");
        }

        printf("|--- %s", p_dirent->d_name);

        /* Itme is a file */
        if (p_dirent->d_type == DT_REG) {
            int curDirentNameLen = strlen(direntName) + strlen(p_dirent->d_name) + 2;

            //prepare next path
            backupDirName = (char *)malloc(curDirentNameLen);
            struct stat *st = NULL;
            st = malloc(sizeof(struct stat));

            if (NULL == backupDirName || NULL == st) {
                goto _err;
            }

            memset(backupDirName, 0, curDirentNameLen);
            memcpy(backupDirName, direntName, strlen(direntName));

            strcat(backupDirName, "/");
            strcat(backupDirName, p_dirent->d_name);

            int statok = stat(backupDirName, st);

            if (0 == statok) {
                printf("[%dB]\n", (int)st->st_size);
            } else {
                printf("\n");
            }

            free(backupDirName);
            backupDirName = NULL;
        } else {
            printf("\n");
        }

        /* Itme is a directory */
        if (p_dirent->d_type == DT_DIR) {
            int curDirentNameLen = strlen(direntName) + strlen(p_dirent->d_name) + 2;

            //prepare next path
            backupDirName = (char *)malloc(curDirentNameLen);

            if (NULL == backupDirName) {
                goto _err;
            }

            memset(backupDirName, 0, curDirentNameLen);
            memcpy(backupDirName, direntName, curDirentNameLen);

            strcat(backupDirName, "/");
            strcat(backupDirName, p_dirent->d_name);

            if (level > 0) {
                TraverseDir(backupDirName, level - 1, indent + 1);
            }

            free(backupDirName);
            backupDirName = NULL;
        }
    }

_err:
    closedir(p_dir);
}

void fm_print_dir(char *direntName, int level)
{
    printf("Traverse directory %s\n", direntName);
    TraverseDir(direntName, level, 0);
    printf("\r\n");
}

const char *fm_get_basepath(void)
{
    return MOUNT_POINT;
}

esp_err_t fm_file_table_create(char *base_path, char ***list_out, uint16_t *files_number, const char *filter_suffix)
{
    DIR *p_dir = NULL;
    struct dirent *p_dirent = NULL;
    p_dir = opendir(base_path);
    if (p_dir == NULL) {
        ESP_LOGE(TAG, "opendir error");
        return ESP_FAIL;
    }

    uint16_t f_num = 0;
    while ((p_dirent = readdir(p_dir)) != NULL) {
        if (p_dirent->d_type == DT_REG) {
            f_num++;
        }
    }

    rewinddir(p_dir);
    *list_out = calloc(f_num, sizeof(char *));
    if (NULL == (*list_out)) {
        ESP_LOGI(TAG, "NO file find, %d", f_num);
        goto _err;
    }
    for (size_t i = 0; i < f_num; i++) {
        (*list_out)[i] = malloc(FLN_MAX);
        if (NULL == (*list_out)[i]) {
            ESP_LOGE(TAG, "malloc failed at %d", i);
            fm_file_table_free(list_out, f_num);
            goto _err;
        }
    }

    uint16_t index = 0;
    while ((p_dirent = readdir(p_dir)) != NULL) {
        if (p_dirent->d_type == DT_REG) {
            if (NULL != filter_suffix) {
                if (strstr(p_dirent->d_name, filter_suffix)) {
                    strncpy((*list_out)[index], p_dirent->d_name, FLN_MAX - 1);
                    (*list_out)[index][FLN_MAX - 1] = '\0';
                    index++;
                }
            } else {
                strncpy((*list_out)[index], p_dirent->d_name, FLN_MAX - 1);
                (*list_out)[index][FLN_MAX - 1] = '\0';
                index++;
            }
        }
    }
    (*files_number) = index;

    closedir(p_dir);
    return ESP_OK;
_err:
    closedir(p_dir);

    return ESP_FAIL;
}

esp_err_t fm_file_table_free(char ***list, uint16_t files_number)
{
    for (size_t i = 0; i < files_number; i++) {
        free((*list)[i]);
    }
    free((*list));
    return ESP_OK;
}

const char *fm_get_filename(const char *file)
{
    const char *p = file + strlen(file);
    while (p > file) {
        if ('/' == *p) {
            return (p + 1);
        }
        p--;
    }
    return file;
}

size_t fm_get_file_size(const char *filepath)
{
    struct stat siz =  { 0 };
    stat(filepath, &siz);
    return siz.st_size;
}
