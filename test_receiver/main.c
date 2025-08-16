#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"

static uint32_t received = 0;
static uint64_t last_ts = 0;

void app_main(void) {
    ESP_LOGI("test_receiver", "Starting test receiver");
    while (1) {
        // Simulate receiving a packet
        uint64_t now = esp_timer_get_time();
        if (last_ts != 0) {
            ESP_LOGI("test_receiver", "Latency: %llu us", now - last_ts);
        }
        last_ts = now;
        received++;
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}
