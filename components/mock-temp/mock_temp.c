#include "mock_temp.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_event_base.h"
#include "esp_timer.h"
#include "portmacro.h"

static const char *TAG = "MOCK_TEMP";

esp_event_loop_handle_t loop_connect;

ESP_EVENT_DEFINE_BASE(TEMP_MOCK);

#define MEASUREMENT_DELAY CONFIG_MEASUREMENT_DELAY

esp_timer_handle_t measurement_timer;

uint64_t us_measurement_delay = MEASUREMENT_DELAY * 1000000u;

enum mock_temp_state temp_state = NOT_INITIALIZED;

float curr_temp;

static void measurement_timer_callback(void * args);

void temp_mock_init(esp_event_loop_handle_t event_loop)
{
    // Sensor logic goes here.
    loop_connect = event_loop;
    temp_state = INITIALIZED;

    const esp_timer_create_args_t measurement_timer_args = {
        .callback = &measurement_timer_callback,
        .name = "measurement"
    };
    esp_timer_create(&measurement_timer_args, &measurement_timer);

    ESP_LOGI(TAG, "Measurement Initialized");
}

esp_err_t temp_mock_start()
{
    esp_timer_start_periodic(measurement_timer, us_measurement_delay);
}

esp_err_t temp_mock_pause()
{
    return esp_timer_stop(measurement_timer);
}

esp_err_t temp_mock_restart()
{
    return esp_timer_restart(measurement_timer, us_measurement_delay);
}

static void measurement_timer_callback(void * arg)
{
    curr_temp += 1.0f;
    ESP_LOGI(TAG, "Temp got new data");
    esp_event_post_to(loop_connect, TEMP_MOCK, TEMP_MOCK_NEW_MEASUREMENT, &curr_temp , sizeof(float), portMAX_DELAY);
}

