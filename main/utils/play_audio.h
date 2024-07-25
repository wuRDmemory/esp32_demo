#pragma once

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "esp_log.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	// The "RIFF" chunk descriptor
	uint8_t ChunkID[4];
	int32_t ChunkSize;
	uint8_t Format[4];
	// The "fmt" sub-chunk
	uint8_t Subchunk1ID[4];
	int32_t Subchunk1Size;
	int16_t AudioFormat;
	int16_t NumChannels;
	int32_t SampleRate;
	int32_t ByteRate;
	int16_t BlockAlign;
	int16_t BitsPerSample;
	// The "data" sub-chunk
	uint8_t Subchunk2ID[4];
	int32_t Subchunk2Size;
} wav_header_t;

/***
 * @brief playing wav file flag
 * @return playing flag
 */
bool get_playing_flag(void);

/***
 * @brief play wav file
 * @param file_name wav file name
 */
void play_sdfile_name(char *base_dir, char *file_name);

/***
 * @brief play spiffs file
 * @param file_name spiffs file name
 */
void play_spiffs_name(char *file_name);

#ifdef __cplusplus
}
#endif