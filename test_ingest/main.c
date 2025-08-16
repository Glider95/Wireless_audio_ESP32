#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "common_audio.h"

#define TEST_PACKET_INTERVAL_MS 1
#define TEST_PACKET_SIZE 256

static uint32_t seq_num = 0;
static uint32_t sample_ctr = 0;
static uint8_t fake_payload[TEST_PACKET_SIZE];

void app_main(void) {
    ESP_LOGI("test_ingest", "Starting fake I2S ingest node");
    memset(fake_payload, 0xA5, sizeof(fake_payload));
    while (1) {
        audio_hdr_t header = {
            .magic = AUDIO_MAGIC,
            .seq_num = seq_num++,
            .sample_ctr = sample_ctr,
            .timestamp_us = esp_timer_get_time(),
            .sd_index = 0,
            .sample_bytes = 3,
            .num_channels = 2,
            .reserved = 0
        };
        sample_ctr += 48 * 2; // 48kHz stereo, 1ms
        // Simulate sending packet (replace with UDP send in real code)
        ESP_LOGI("test_ingest", "Fake packet: seq=%u, ts=%u", header.seq_num, header.timestamp_us);
        vTaskDelay(pdMS_TO_TICKS(TEST_PACKET_INTERVAL_MS));
    }
}
