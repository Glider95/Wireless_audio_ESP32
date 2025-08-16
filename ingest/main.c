// ingest/main.c
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s_std.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "lwip/sockets.h"
#include "common_audio.h"

#define WIFI_SSID   "WiSA-DIY"
#define WIFI_PASS   "12345678"
#define AGGR_IP     "192.168.4.1"   // Aggregator AP default gateway
#define SD_INDEX    0               // <-- build four firmwares with 0,1,2,3

// Pin mapping example (edit as you wired)
#define PIN_BCLK    GPIO_NUM_10
#define PIN_LRCLK   GPIO_NUM_11
#define PIN_SDIN    GPIO_NUM_12     // change per board if you routed differently

static i2s_chan_handle_t rx_chan;
static int sock;
static struct sockaddr_in aggr_addr;
static uint32_t seq_num = 0;
static uint32_t sample_ctr = 0;

static void wifi_sta_init(void) {
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    wifi_config_t sta = { 0 };
    strncpy((char*)sta.sta.ssid, WIFI_SSID, sizeof(sta.sta.ssid));
    strncpy((char*)sta.sta.password, WIFI_PASS, sizeof(sta.sta.password));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());
}

static void udp_init(void) {
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    aggr_addr.sin_family = AF_INET;
    aggr_addr.sin_port = htons(UDP_PORT_INGEST);
    aggr_addr.sin_addr.s_addr = inet_addr(AGGR_IP);
}

static void i2s_rx_init(void) {
    i2s_new_channel(&(i2s_chan_config_t) {
        .id = I2S_NUM_0, .role = I2S_ROLE_SLAVE, .dma_desc_num = 8,
        .dma_frame_num = (SAMPLE_RATE/1000)*CHANNELS_PER_PKT
    }, NULL, &rx_chan);

    i2s_std_config_t cfg = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(
                        I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = PIN_BCLK,
            .ws   = PIN_LRCLK,
            .dout = I2S_GPIO_UNUSED,
            .din  = PIN_SDIN,
        },
    };
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_chan, &cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(rx_chan));
}

void app_main(void) {
    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_sta_init();
    udp_init();
    i2s_rx_init();

    const size_t frames = SAMPLE_RATE/1000;
    const size_t bytes_per_frame = CHANNELS_PER_PKT*(BITS_PER_SAMPLE/8);
    const size_t payload_len = frames*bytes_per_frame;

    uint8_t payload[payload_len];
    uint8_t pkt[sizeof(audio_hdr_t)+payload_len];

    while (1) {
        size_t n = 0;
        ESP_ERROR_CHECK(i2s_channel_read(rx_chan, payload, payload_len, &n, portMAX_DELAY));

        audio_hdr_t *h = (audio_hdr_t*)pkt;
        memcpy(h->magic, "AUD0", 4);
        h->seq_num       = seq_num++;
        h->sample_ctr    = sample_ctr;
        h->timestamp_us  = (uint32_t)esp_timer_get_time();
        h->sd_index      = SD_INDEX;
        h->sample_bytes  = BITS_PER_SAMPLE/8;
        h->num_channels  = CHANNELS_PER_PKT;

        memcpy(pkt+sizeof(audio_hdr_t), payload, n);
        sendto(sock, pkt, sizeof(audio_hdr_t)+n, 0,
               (struct sockaddr*)&aggr_addr, sizeof(aggr_addr));

        sample_ctr += frames;
    }
}
