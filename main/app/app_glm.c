#include "app_glm.h"
#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "esp_tls.h"
#include "esp_log.h"
#include "../custom_jwt/custom_jwt.h"
#include "../utils/utils.h"

#define TAG "[GLM]"
#define MAX_HTTP_RECV_BUFFER 2048
#define MAX_JWT_PAYLOAD_LEN 256

static const char *host = "https://open.bigmodel.cn/api/paas/v4/chat/completions";
static const char *key_id = "c5e87a8dfeb8a4c5b17396166803f5fb";
static const char *key_secret = "cjLc3oAQJpZzerfP";

static char glm_payload[128];
static char *jwt_payloadJSON;

static esp_http_client_handle_t client;

void app_glm_initialise(void) {
    // config jwt
    setCustomJWT(key_secret, MAX_JWT_PAYLOAD_LEN, 40, 32, "HS256", "SIGN");
    allocateJWTMemory();
}

void app_glm_task(void *pvParameters) {
    while (1) {
        // config http client
        esp_http_client_config_t config = {
            .url = host,
            .auth_type = HTTP_AUTH_TYPE_DIGEST,
            .method = HTTP_METHOD_POST,
            .transport_type = HTTP_TRANSPORT_OVER_TCP,
            .crt_bundle_attach = esp_crt_bundle_attach,
        };
        client = esp_http_client_init(&config);

        // config glm payload
        jwt_payloadJSON = (char *)malloc(MAX_JWT_PAYLOAD_LEN);
        double unix_timestamp = get_unix_timestamp();
        uint64_t timestamp = (uint64_t)(unix_timestamp * 1000);
        uint64_t exp = timestamp + 10000;
        sprintf(jwt_payloadJSON, "{\"api_key\": \"%s\", \"exp\": %lld, \"timestamp\": %lld}", \
            key_id, exp, timestamp);

        // encode jwt
        if (!encodeJWT(jwt_payloadJSON)) {
            ESP_LOGE(TAG, "Failed to encode JWT");
            return;
        }
        free(jwt_payloadJSON);
        jwt_payloadJSON = NULL;

        char bearer[256 + 8];
        memset(bearer, 0, 256 + 8);
        sprintf(bearer, "Bearer %s", jwt_out);
        esp_http_client_set_header(client, "Content-Type", "application/json");
        esp_http_client_set_header(client, "Authorization", bearer);

        // communicate with glm
        const char *post_data = "{\"messages\":[{\"role\":\"user\", \"content\":\"你好\"}], \"model\": \"glm-3-turbo\"}";
        esp_err_t err = esp_http_client_open(client, strlen(post_data));
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
            return;
        }

        // post messages
        int wlen = esp_http_client_write(client, post_data, strlen(post_data));
        if (wlen < 0) {
            ESP_LOGE(TAG, "Write failed");
            return;
        }

        int content_length = esp_http_client_fetch_headers(client);
        if (content_length < 0) {
            ESP_LOGE(TAG, "HTTP client fetch headers failed");
        } else {
            char *buffer = (char *)malloc(MAX_HTTP_RECV_BUFFER);
            int data_read = esp_http_client_read_response(client, buffer, MAX_HTTP_RECV_BUFFER);
            if (data_read >= 0) {
                ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
                ESP_LOGI(TAG, "%s", buffer);
            } else {
                ESP_LOGE(TAG, "Failed to read response");
            }
            free(buffer);
            buffer = NULL;
        }
        
        esp_http_client_cleanup(client);

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}