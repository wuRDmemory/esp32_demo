#include "app_camera.h"
#include "esp_timer.h"

#define MAX_HTTP_RECV_BUFFER 512

#define CAM_PIN_PWDN -1
#define CAM_PIN_RESET -1
#define CAM_PIN_XCLK -1
#define CAM_PIN_SIOD 17
#define CAM_PIN_SIOC 18

#define CAM_PIN_D7 39
#define CAM_PIN_D6 41
#define CAM_PIN_D5 42
#define CAM_PIN_D4 5
#define CAM_PIN_D3 40
#define CAM_PIN_D2 14
#define CAM_PIN_D1 47
#define CAM_PIN_D0 45
#define CAM_PIN_VSYNC 21
#define CAM_PIN_HREF 38
#define CAM_PIN_PCLK 48
#define PART_BOUNDARY "123456789000000000000987654321"

static const char* TAG = "CAMERA";
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

static char deviceUUID[17];
static camera_config_t camera_config = {.pin_pwdn = CAM_PIN_PWDN,
                                        .pin_reset = CAM_PIN_RESET,
                                        .pin_xclk = CAM_PIN_XCLK,
                                        .pin_sscb_sda = CAM_PIN_SIOD,
                                        .pin_sscb_scl = CAM_PIN_SIOC,

                                        .pin_d7 = CAM_PIN_D7,
                                        .pin_d6 = CAM_PIN_D6,
                                        .pin_d5 = CAM_PIN_D5,
                                        .pin_d4 = CAM_PIN_D4,
                                        .pin_d3 = CAM_PIN_D3,
                                        .pin_d2 = CAM_PIN_D2,
                                        .pin_d1 = CAM_PIN_D1,
                                        .pin_d0 = CAM_PIN_D0,
                                        .pin_vsync = CAM_PIN_VSYNC,
                                        .pin_href = CAM_PIN_HREF,
                                        .pin_pclk = CAM_PIN_PCLK,

                                        // XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
                                        .xclk_freq_hz = 20000000,
                                        .ledc_timer = LEDC_TIMER_0,
                                        .ledc_channel = LEDC_CHANNEL_0,
                                        // .fb_location = CAMERA_FB_IN_PSRAM,
                                        .fb_location = CAMERA_FB_IN_DRAM,
                                        .pixel_format = PIXFORMAT_JPEG, // YUV422,GRAYSCALE,RGB565,JPEG
                                        .frame_size = FRAMESIZE_VGA,    // QQVGA-UXGA Do not use sizes above QVGA when not JPEG

                                        .jpeg_quality = 12, // 0-63 lower number means higher quality
                                        .fb_count = 2,      // if more than one, i2s runs in continuous mode. Use only with JPEG
                                        .grab_mode = CAMERA_GRAB_WHEN_EMPTY};

static esp_err_t http_get_handler(httpd_req_t* req) {

	// 获取芯片可用内存
	printf("esp_get_free_heap_size : %d  \n", esp_get_free_heap_size());

	camera_fb_t* fb = NULL;
	esp_err_t res = ESP_OK;
	size_t _jpg_buf_len;
	uint8_t* _jpg_buf;
	char* part_buf[64];
	static int64_t last_frame = 0;
	if (!last_frame) {
		last_frame = esp_timer_get_time();
	}

	res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
	if (res != ESP_OK) {
		return res;
	}

	while (true) {
		fb = esp_camera_fb_get(); // 获取图片
		if (!fb) {
			ESP_LOGE(TAG, "Camera capture failed");
			res = ESP_FAIL;
			break;
		}
		if (fb->format != PIXFORMAT_JPEG) {
            
			bool jpeg_converted = frame2jpg(fb, 50, &_jpg_buf, &_jpg_buf_len);
			if (!jpeg_converted) {
				ESP_LOGE(TAG, "JPEG compression failed");
				esp_camera_fb_return(fb);
				res = ESP_FAIL;
			}
		} else {
			_jpg_buf_len = fb->len;
			_jpg_buf = fb->buf;
		}

		if (res == ESP_OK) {
			res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
		}
		if (res == ESP_OK) {
			size_t hlen = snprintf((char*)part_buf, 64, _STREAM_PART,
			                       _jpg_buf_len); // 发送图片尺寸大小

			res = httpd_resp_send_chunk(req, (const char*)part_buf, hlen);
		}
		if (res == ESP_OK) {
			res = httpd_resp_send_chunk(req, (const char*)_jpg_buf,
			                            _jpg_buf_len); // 发送图片
		}
		if (fb->format != PIXFORMAT_JPEG) {
			free(_jpg_buf);
		}
		esp_camera_fb_return(fb);
		if (res != ESP_OK) {
			break;
		}
		int64_t fr_end = esp_timer_get_time();
		int64_t frame_time = fr_end - last_frame;
		last_frame = fr_end;
		frame_time /= 1000;
		ESP_LOGI(TAG, "MJPG: %uKB %ums (%.1ffps)", (uint32_t)(_jpg_buf_len / 1024), (uint32_t)frame_time, 1000.0 / (uint32_t)frame_time);
	}

	last_frame = 0;
	return res;
}

// 只要进入/stream 页面就进入http_get_handler 这个函数
static httpd_uri_t stream = {.uri = "/stream",
                             .method = HTTP_GET,
                             .handler = http_get_handler,
                             /* Let's pass response string in user
                              * context to demonstrate it's usage */
                             .user_ctx = "stream!"};

httpd_handle_t camera_server = NULL;

esp_err_t app_camera_initialise() {

    //获取mac地址（station模式）
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    sprintf(deviceUUID, "%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

	// initialize the camera
	esp_err_t err = esp_camera_init(&camera_config);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "Camera Init Failed");
		return err;
	}

	ESP_LOGI(TAG, "Camera Init Success");
	return ESP_OK;
}

void camera_task(void* pvParameters) {

	ESP_LOGI(TAG, "Http Start");
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();

	// Start the httpd server
	ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
	if (httpd_start(&camera_server, &config) == ESP_OK) {
		// Set URI handlers
		ESP_LOGI(TAG, "Registering URI handlers");
		httpd_register_uri_handler(camera_server, &stream);
	} else {
		ESP_LOGI(TAG, "Error starting server!");
	}

	ESP_LOGI(TAG, "Http End");
	vTaskDelete(NULL);
}

void stop_camera_task(void) {
	// Stop the httpd server
	httpd_stop(camera_server);
}
