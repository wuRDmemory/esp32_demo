#include "play_audio.h"
#include "file_manager.h"
#include "driver/i2s.h"
#include "../hardware/bsp_i2s.h"

static const char *TAG = "PLAY_AUDIO";

static bool first_play = true;
static bool playing = false;

static char base_buf[128] = {0};
static char path_buf[256] = {0};

static char **g_file_list = NULL;
static uint16_t g_file_num = 0;

esp_err_t play_wav(const char *filepath)
{
	disable_all_i2s();
	play_i2s_initialise();
	FILE *fd = NULL;
	struct stat file_stat;

	if (stat(filepath, &file_stat) == -1)//先找找这个文件是否存在
	{
		ESP_LOGE(TAG, "Failed to stat file : %s", filepath);
		disable_all_i2s();
		record_i2s_initialise();
		return ESP_FAIL;//如果不存在就继续录音
	}

	ESP_LOGI(TAG, "file stat info: %s (%ld bytes)...", filepath, file_stat.st_size);
	fd = fopen(filepath, "r");
	if (NULL == fd)
	{
		ESP_LOGE(TAG, "Failed to read existing file : %s", filepath);
		disable_all_i2s();
		record_i2s_initialise();
		return ESP_FAIL;
	}
	const size_t chunk_size = 4096;
	uint8_t *buffer = malloc(chunk_size);

	if (NULL == buffer)
	{
		ESP_LOGE(TAG, "audio data buffer malloc failed");
		disable_all_i2s();
		record_i2s_initialise();
		fclose(fd);
		return ESP_FAIL;
	}

	/**
	 * read head of WAV file
	 */
	wav_header_t wav_head;
	int len = fread(&wav_head, 1, sizeof(wav_header_t), fd);//读取wav文件的文件头
	if (len <= 0)
	{
		ESP_LOGE(TAG, "Read wav header failed");
		disable_all_i2s();
		record_i2s_initialise();
		fclose(fd);
		return ESP_FAIL;
	}
	if (NULL == strstr((char *)wav_head.Subchunk1ID, "fmt") &&
		NULL == strstr((char *)wav_head.Subchunk2ID, "data"))
	{
		fclose(fd);
		return ESP_FAIL;
	}

	ESP_LOGI(TAG, "frame_rate=%d, ch=%d, width=%d", wav_head.SampleRate, wav_head.NumChannels, wav_head.BitsPerSample);

	/**
	 * read wave data of WAV file
	 */
	size_t write_num = 0;
	size_t cnt;
	i2s_set_clk(1, wav_head.SampleRate, wav_head.BitsPerSample, 1);//根据该wav文件的各种参数来配置一下i2S的clk 采样率等等
	do
	{
		/* Read file in chunks into the scratch buffer */
		len = fread(buffer, 1, chunk_size, fd);
		if (len <= 0)
		{
			break;
		}
		i2s_write(1, buffer, len, &cnt, 1000 / portTICK_PERIOD_MS);
		write_num += len;
	} while (1);
	fclose(fd);
	ESP_LOGI(TAG, "File reading complete, total: %d bytes", write_num);

    disable_all_i2s();
    record_i2s_initialise();
	return ESP_OK;
}

bool get_playing_flag(void)
{
    return playing;
}

void set_playing_flag(bool flag)
{
    playing = flag;
}

void play_sdfile_name(char* dir_name, char *file_name)
{
	playing = true;
    if (first_play) {
        first_play = false;
        sprintf(base_buf, "%s/%s", MOUNT_POINT, dir_name);
        fm_file_table_create(base_buf, &g_file_list, &g_file_num, ".wav");
        for (size_t i = 0; i < g_file_num; i++)
        {
            ESP_LOGI(TAG, "have file [%d:%s]", i, g_file_list[i]);
        }

        if (0 == g_file_num)
        {
            ESP_LOGW(TAG, "Can't found any wav file in sdcard!");
            playing = false;
            return;
        }
    }

	sprintf(path_buf, "%s/%s", base_buf, file_name);
	play_wav(path_buf);
	// fm_file_table_free(&g_file_list, g_file_num);
    vTaskDelay(10);
	playing = false;
}

void play_spiffs_name(char *file_name)
{

}
