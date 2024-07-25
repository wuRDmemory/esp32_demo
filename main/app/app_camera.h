#ifndef __APP_CAMERA_H__
#define __APP_CAMERA_H__

#include "esp_log.h"
#include "esp_system.h"
#include "esp_camera.h"
#include "esp_http_server.h"
#include "http_server.h"

extern httpd_handle_t camera_server;

/**
 * @brief initialize camera
 * @param void
 * @return esp_err_t
*/
esp_err_t app_camera_initialise();


/**
 * @brief camera task, build serve and wait for connection
 * @param pvParameter
 * @return void
*/
void camera_task(void *pvParameter);

/**
 * @brief stop camera task
 * @param void
 * @return void
*/
void stop_camera_task(void);

#endif