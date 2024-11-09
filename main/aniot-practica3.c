#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "esp_err.h"
#include "esp_event_base.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/idf_additions.h"
#include "mock_wifi.h"
#include "mock_flash.h"
#include "mock_temp.h"
#include "esp_event.h"
/* TODO implement a component to describe STATE_EVENTs */
/* #include "state_event.h" */

#define MAX_FLASH_CAPACITY 1024

static char *TAG = "MAIN APP";
SemaphoreHandle_t monitoring;

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


static void temp_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispached from event loop base=%s, event_id=%" PRIi32
             "", base, event_id);
    float temp = *((float*) event_data);
    switch ((temp_event_id_t) event_id) {
        case TEMP_MOCK_NEW_MEASUREMENT:
            send_or_store(&temp, 1);
            break;

        default:
            ESP_LOGD(TAG, "Unhandled event_id");
            break;
    }
}

static void wifi_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispached from event loop base=%s, event_id=%" PRIi32
             "", base, event_id);
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

static void state_event_handler(void * handler_args, esp_event_base_t base,
                                int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispached from event loop base=%s, event_id=%" PRIi32
             "", base, event_id);
    switch ((state_event_id_t)event_id) {
        case STATE_EVENT_MONITORING:
            ESP_ERROR_CHECK(esp_event_handler_register_with(h_operation_event_loop,
                                                            TEMP_MOCK,
                                                            ESP_EVENT_ANY_ID,
                                                            temp_event_handler,
                                                            NULL));

            ESP_ERROR_CHECK(esp_event_handler_register_with(h_operation_event_loop,
                                                            WIFI_MOCK,
                                                            ESP_EVENT_ANY_ID,
                                                            wifi_event_handler,
                                                            NULL));
            break;

        case STATE_EVENT_CONSOLE:
            /* If we mirror the autoreconnect functionality of wifi on the temp
             * sensor it could save us on manually reconnecting on monitoring*/
            ESP_ERROR_CHECK(esp_event_handler_unregister_with(h_operation_event_loop,
                                                            TEMP_MOCK,8
                                                            ESP_EVENT_ANY_ID,
                                                            temp_event_handler,
                                                            NULL));

            /* If we call wifi_disconnect() prior to unregistering, it will 
             * attempt to connect back up inside the handler*/
            ESP_ERROR_CHECK(esp_event_handler_unregister_with(h_operation_event_loop,
                                                            WIFI_MOCK,
                                                            ESP_EVENT_ANY_ID,
                                                            wifi_event_handler,
                                                            NULL));
            /* This should still generate a WIFI_MOCK_EVENT_WIFI_DISCONNECTED
             * so that once the handler gets registered again it should connect
             * automatically, if it doesn't that logic needs to get added */
            wifi_disconnect();
            temp_mock_pause();
            /* TODO! implement console logic */
            break;
            
}

void app_main(void)
{
    ESP_ERROR_CHECK(mock_flash_init(MAX_FLASH_CAPACITY));

    esp_event_loop_handle_t h_operation_event_loop;
    esp_event_loop_args_t operation_event_loop_args = {
        .queue_size =5,
        .task_name = "operation_task", /* since it is a task it can be stopped */
        .task_stack_size = 1024,
        .task_priority = uxTaskPriorityGet(NULL),
        .task_core_id = tskNO_AFFINITY,
    };
    esp_event_loop_handle_t h_state_event_loop;
    esp_event_loop_args_t state_event_loop_args = {
        .queue_size =5,
        .task_name = "state_switcher_task", /* since it is a task it can be stopped */
        .task_stack_size = 1024,
        .task_priority = uxTaskPriorityGet(NULL),
        .task_core_id = tskNO_AFFINITY,
    };
    ESP_ERROR_CHECK(esp_event_loop_create(&operation_event_loop_args,
                                          &h_operation_event_loop));

    ESP_ERROR_CHECK(esp_event_loop_create(&state_event_loop_args,
                                          &h_state_event_loop));
}
