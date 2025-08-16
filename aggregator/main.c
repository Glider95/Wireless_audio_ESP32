// aggregator/main.c
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "lwip/sockets.h"
#include "common_audio.h"

#define AP_SSID  "WiSA-DIY"
#define AP_PASS  "12345678"
#define AP_CHAN  36

typedef struct {
    int   out_sock;
    struct sockaddr_in out_addr;
    uint32_t out_seq_base;
} lane_out_t;

static int in_sock;
static lane_out_t lane[4]; // SD0..SD3

static void wifi_ap_init(void){
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_ap();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t ap = { 0 };
    strncpy((char*)ap.ap.ssid, AP_SSID, sizeof(ap.ap.ssid));
    ap.ap.channel = AP_CHAN;
    strncpy((char*)ap.ap.password, AP_PASS, sizeof(ap.ap.password));
    ap.ap.max_connection = 8;
    ap.ap.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap));
    ESP_ERROR_CHECK(esp_wifi_start());
}

static void udp_in_init(void){
    in_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    struct sockaddr_in a = {0};
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(INADDR_ANY);
    a.sin_port = htons(UDP_PORT_INGEST);
    bind(in_sock, (struct sockaddr*)&a, sizeof(a));
}

static void lane_out_init(uint8_t sd_index){
    lane[sd_index].out_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    lane[sd_index].out_addr.sin_family = AF_INET;
    lane[sd_index].out_addr.sin_port = htons(UDP_PORT_CAST);
    char ip[16];
    snprintf(ip, sizeof(ip), "%s%d", MCAST_BASE_IP, 11 + sd_index);
    lane[sd_index].out_addr.sin_addr.s_addr = inet_addr(ip);
    lane[sd_index].out_seq_base = 0;
}

void app_main(void){
    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_ap_init();
    udp_in_init();
    for(int i=0;i<4;i++) lane_out_init(i);

    // re-packetize loop (simple pass-through + re-seq)
    uint8_t rxbuf[1600];
    while(1){
        int len = recv(in_sock, rxbuf, sizeof(rxbuf), 0);
        if(len <= (int)sizeof(audio_hdr_t)) continue;

        audio_hdr_t *h = (audio_hdr_t*)rxbuf;
        if (memcmp(h->magic,"AUD0",4)!=0 || h->sd_index>3) continue;

        // re-stamp seq so all lanes share a monotonic global sequence
        static uint32_t global_seq = 0;
        h->seq_num      = global_seq++;
        h->timestamp_us = (uint32_t)esp_timer_get_time();

        sendto(lane[h->sd_index].out_sock, rxbuf, len, 0,
               (struct sockaddr*)&lane[h->sd_index].out_addr, sizeof(lane[h->sd_index].out_addr));
    }
}
