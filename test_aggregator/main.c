#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"

static uint32_t packet_count = 0;
static uint64_t start_time = 0;

void app_main(void) {
    ESP_LOGI("test_aggregator", "Starting test aggregator");
    start_time = esp_timer_get_time();
    while (1) {
        // Simulate receiving a packet every 1ms
        packet_count++;
        if (packet_count % 1000 == 0) {
            uint64_t now = esp_timer_get_time();
            double seconds = (now - start_time) / 1e6;
            double bandwidth = (packet_count * 256 * 8) / seconds / 1e6; // Mbps
            ESP_LOGI("test_aggregator", "Packets: %u, Bandwidth: %.2f Mbps", packet_count, bandwidth);
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}
