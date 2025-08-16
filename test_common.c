#include "test_common.h"
#include "nvs_flash.h"
#include "esp_netif.h"

static const char* TAG = "test_common";

esp_err_t wifi_init(bool is_ap) {
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    if (is_ap) {
        esp_netif_create_default_wifi_ap();
        wifi_config_t wifi_config = {
            .ap = {
                .ssid = WIFI_SSID,
                .ssid_len = strlen(WIFI_SSID),
                .password = WIFI_PASS,
                .max_connection = 4,
                .authmode = WIFI_AUTH_WPA2_PSK,
                .channel = 36  // 5GHz channel
            },
        };
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    } else {
        esp_netif_create_default_wifi_sta();
        wifi_config_t wifi_config = {
            .sta = {
                .ssid = WIFI_SSID,
                .password = WIFI_PASS,
            },
        };
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    }
    
    ESP_ERROR_CHECK(esp_wifi_start());
    return ESP_OK;
}

int udp_socket_init(uint16_t port, bool is_multicast) {
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        return -1;
    }

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr.s_addr = htonl(INADDR_ANY)
    };

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        close(sock);
        return -1;
    }

    if (is_multicast) {
        struct ip_mreq mreq = {
            .imr_interface.s_addr = INADDR_ANY,
        };
        // Join multicast group for each channel
        for (int i = 0; i < 4; i++) {
            char mcast_addr[16];
            snprintf(mcast_addr, sizeof(mcast_addr), "%s%d", MCAST_BASE_IP, 11 + i);
            inet_aton(mcast_addr, &mreq.imr_multiaddr);
            setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
        }
    }

    return sock;
}

void update_performance_stats(performance_stats_t* stats, const audio_hdr_t* packet) {
    uint64_t now = esp_timer_get_time();
    
    // Update packet count
    stats->packet_count++;
    
    // Calculate latency
    if (stats->last_timestamp != 0) {
        float latency = (now - packet->timestamp_us) / 1000.0f; // ms
        stats->average_latency_ms = (stats->average_latency_ms * 0.95f) + (latency * 0.05f);
    }
    stats->last_timestamp = now;
    
    // Calculate bandwidth (rolling average)
    float instant_bandwidth = (sizeof(audio_hdr_t) + MAX_PACKET_SIZE) * 8.0f / 1000000.0f; // Mbps
    stats->current_bandwidth_mbps = (stats->current_bandwidth_mbps * 0.95f) + (instant_bandwidth * 0.05f);
}

void print_performance_report(const performance_stats_t* stats) {
    ESP_LOGI(TAG, "Performance Report:");
    ESP_LOGI(TAG, "Packets Received: %u", stats->packet_count);
    ESP_LOGI(TAG, "Packets Lost: %u", stats->lost_packets);
    ESP_LOGI(TAG, "Average Latency: %.2f ms", stats->average_latency_ms);
    ESP_LOGI(TAG, "Current Bandwidth: %.2f Mbps", stats->current_bandwidth_mbps);
    ESP_LOGI(TAG, "Buffer Level: %u%%", (stats->buffer_level * 100) / RING_BUFFER_SIZE);
}
