#ifndef MOCK_TEMP_H_
#define MOCK_TEMP_H_

#include "esp_err.h"
#include "esp_event_base.h"

ESP_EVENT_DECLARE_BASE(TEMP_MOCK);
typedef enum
{
    TEMP_MOCK_NEW_MEASUREMENT,
} temp_event_id_t ;

void temp_mock_init(esp_event_loop_handle_t event_loop);

esp_err_t temp_mock_start(void);

esp_err_t temp_mock_pause(void);

#endif // !MOCK_TEMP_H_
