// receiver/main.c
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s_std.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "lwip/sockets.h"
#include "lwip/igmp.h"
#include "common_audio.h"

#define WIFI_SSID   "WiSA-DIY"
#define WIFI_PASS   "12345678"
#define MCAST_IP    "239.255.0.11"   // set .11/.12/.13/.14 for SD0..SD3

// Pins for I²S out to DAC
#define PIN_BCLK    GPIO_NUM_10
#define PIN_LRCLK   GPIO_NUM_11
#define PIN_DOUT    GPIO_NUM_12

static i2s_chan_handle_t tx_chan;
static int sock;
static uint8_t ring[ (12) * (SAMPLE_RATE/1000) * CHANNELS_PER_PKT * (BITS_PER_SAMPLE/8) ];
static volatile size_t wpos=0, rpos=0;
static volatile bool started=false;

static void wifi_sta_init(void){
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    wifi_config_t sta={0};
    strncpy((char*)sta.sta.ssid, WIFI_SSID, sizeof(sta.sta.ssid));
    strncpy((char*)sta.sta.password, WIFI_PASS, sizeof(sta.sta.password));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());
}

static void udp_join(void){
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    struct sockaddr_in a={0};
    a.sin_family = AF_INET;
    a.sin_port = htons(UDP_PORT_CAST);
    a.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(sock, (struct sockaddr*)&a, sizeof(a));

    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(MCAST_IP);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
}

static void i2s_tx_init(void){
    i2s_new_channel(&(i2s_chan_config_t){
        .id=I2S_NUM_0, .role=I2S_ROLE_MASTER, .dma_desc_num=8,
        .dma_frame_num=(SAMPLE_RATE/1000)*CHANNELS_PER_PKT
    }, &tx_chan, NULL);

    i2s_std_config_t cfg = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(
                       I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = { .mclk=I2S_GPIO_UNUSED, .bclk=PIN_BCLK, .ws=PIN_LRCLK,
                      .dout=PIN_DOUT, .din=I2S_GPIO_UNUSED }
    };
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_chan, &cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(tx_chan));
}

static void net_rx_task(void*){
    uint8_t buf[1600];
    while(1){
        int n = recv(sock, buf, sizeof(buf), 0);
        if(n <= (int)sizeof(audio_hdr_t)) continue;
        audio_hdr_t *h = (audio_hdr_t*)buf;
        if (memcmp(h->magic,"AUD0",4)!=0) continue;

        uint8_t *pl = buf + sizeof(audio_hdr_t);
        size_t pl_len = n - sizeof(audio_hdr_t);

        for(size_t i=0;i<pl_len;i++){
            ring[wpos++] = pl[i];
            if(wpos>=sizeof(ring)) wpos=0;
        }
        if(!started){
            size_t filled = (wpos>=rpos) ? (wpos-rpos) : (sizeof(ring)-rpos+wpos);
            if(filled >= sizeof(ring)*9/10) started=true; // ~10–12 ms
        }
    }
}

static void audio_task(void*){
    const size_t frame_bytes = (SAMPLE_RATE/1000)*CHANNELS_PER_PKT*(BITS_PER_SAMPLE/8);
    uint8_t out[frame_bytes];
    while(1){
        if(!started){ vTaskDelay(pdMS_TO_TICKS(1)); continue; }
        for(size_t i=0;i<frame_bytes;i++){
            out[i] = ring[rpos++]; if(rpos>=sizeof(ring)) rpos=0;
        }
        size_t w=0; i2s_channel_write(tx_chan, out, frame_bytes, &w, portMAX_DELAY);
    }
}

void app_main(void){
    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_sta_init();
    udp_join();
    i2s_tx_init();
    xTaskCreate(net_rx_task, "net_rx", 4096, NULL, 5, NULL);
    xTaskCreate(audio_task, "audio", 4096, NULL, 5, NULL);
}
