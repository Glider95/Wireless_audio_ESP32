#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"

void app_main(void)
{
    ESP_LOGI("wireless_audio", "Hello, ESP32C5 wireless audio project!");
    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
