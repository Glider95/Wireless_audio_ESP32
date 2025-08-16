#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "common_audio.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_timer.h"

// Network configuration for testing
#define WIFI_SSID "WiSA-DIY-Test"
#define WIFI_PASS "test12345"
#define TEST_PORT 5000

// Buffer sizes
#define TEST_MAX_BUFFER 1024  // Maximum size for test buffers
#define RING_BUFFER_SIZE 32

// Performance monitoring
typedef struct {
    uint64_t last_timestamp;
    uint32_t packet_count;
    uint32_t lost_packets;
    float average_latency_ms;
    float current_bandwidth_mbps;
    uint32_t buffer_level;
} performance_stats_t;

// Initialize WiFi as AP (for aggregator) or station (for ingest/receiver)
esp_err_t wifi_init(bool is_ap);

// Initialize UDP socket
int udp_socket_init(uint16_t port, bool is_multicast);

// Monitor and log performance
void update_performance_stats(performance_stats_t* stats, const audio_hdr_t* packet);

// Print performance report
void print_performance_report(const performance_stats_t* stats);

#endif // TEST_COMMON_H
