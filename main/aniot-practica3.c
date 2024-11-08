#include <stdint.h>
#include <stdio.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/idf_additions.h"
#include "mock_wifi.h"
#include "mock_flash.h"
#include "mock_temp.h"
#include "esp_event.h"

#define MAX_FLASH_CAPACITY 1024

static char *TAG = "MAIN APP";

static void send_or_store(void *data, size_t len)
{
    esp_err_t send_state = send_data_wifi(data, len);
    switch (send_state) {
        case ESP_ERR_INVALID_STATE:
            writeToFlash(data, len);
        break;

        case ESP_OK:
            ESP_LOGD(TAG, "Temperature measurement correctly sent using wifi");
            break;

        default:
            ESP_LOGD(TAG, "Unhandled return state");
            break;
    }
}


static void temp_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispached from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    float temp = *((float*) event_data);
    esp_err_t send_state;
    switch ((temp_event_id_t) event_id) {
        case TEMP_MOCK_NEW_MEASUREMENT:
            send_or_store(&temp, 1);
            break;

        default:
            ESP_LOGD(TAG, "Unhandled event_id");
            break;
    }
}

static void wifi_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispached from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_err_t send_state;
    size_t data_left;
    void *flashContents;
    switch ((wifi_event_id_t)event_id) {
        case WIFI_MOCK_EVENT_WIFI_CONNECTED:
            ESP_LOGI("TAG", "Wifi connected, waiting for IP");
            break;

        case WIFI_MOCK_EVENT_WIFI_GOT_IP:
            data_left = getDataLeft();
            flashContents = readFromFlash(data_left);
            send_or_store(flashContents, data_left);
            break;

        case WIFI_MOCK_EVENT_WIFI_DISCONNECTED:
            wifi_connect();

    }

}

void app_main(void)
{
    esp_event_loop_handle_t h_wifi_event_loop;
    esp_event_loop_args_t wifi_event_loop_args = {
        .queue_size =5,
        .task_name = NULL, /* since it is a task it can be stopped */
    };
    ESP_ERROR_CHECK(esp_event_loop_create(&wifi_event_loop_args, &h_wifi_event_loop));
    esp_event_loop_handle_t h_temp_event_loop;
    esp_event_loop_args_t temp_event_loop_args = {
        .queue_size = 5,
        .task_name = NULL,
    };
    ESP_ERROR_CHECK(esp_event_loop_create(&temp_event_loop_args, &h_temp_event_loop));
    ESP_ERROR_CHECK(mock_flash_init(MAX_FLASH_CAPACITY));
}
