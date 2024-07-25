#ifndef __APP_GLM_H__
#define __APP_GLM_H__

#include <stdio.h>

extern int8_t rcv_data;
extern char *output_buffer;

/**
 * @brief glm task init
*/
void app_glm_initialise(void);

/**
 * @brief glm task
*/
void app_glm_task(void *pvParameters);

#endif
