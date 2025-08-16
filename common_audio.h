// common_audio.h
#pragma once
#include <stdint.h>

#define SAMPLE_RATE       48000
#define BITS_PER_SAMPLE   24
#define CHANNELS_PER_PKT  2        // stereo per SDx
#define MS_PER_PKT        1        // 1 ms packets

#define UDP_PORT_INGEST   6000     // ingest->aggregator unicast
#define UDP_PORT_CAST     5000     // aggregator->receivers multicast
#define MCAST_BASE_IP     "239.255.0." // aggregator will use .11, .12, .13, .14
#define AUDIO_MAGIC       {'A','U','D','0'}      // Magic number for audio packets

// sd_index mapping for SoundSend-style wiring
// 0: SD0 (Front L/R), 1: SD1 (Center/Sub), 2: SD2 (Surround L/R), 3: SD3 (Rear L/R)
typedef struct {
    char     magic[4];      // 'AUD0'
    uint32_t seq_num;       // increasing per 1 ms block
    uint32_t sample_ctr;    // total frames since boot on this SDx
    uint32_t timestamp_us;  // esp_timer_get_time() at capture
    uint8_t  sd_index;      // 0..3 identifies SDx lane
    uint8_t  sample_bytes;  // 3 for 24-bit (packed in 32-bit slots)
    uint8_t  num_channels;  // 2
    uint8_t  reserved;
} __attribute__((packed)) audio_hdr_t;

// Add quality monitoring
typedef struct {
    uint32_t packets_received;
    uint32_t packets_lost;
    uint32_t buffer_underruns;
    uint32_t buffer_overruns;
    float average_latency_ms;
} quality_metrics_t;

// Add circular buffer for jitter management
#define BUFFER_SIZE 32
#define MAX_PACKET_SIZE 256  // Maximum size of an audio packet (header + payload)

typedef struct {
    audio_hdr_t headers[BUFFER_SIZE];
    uint8_t data[BUFFER_SIZE][MAX_PACKET_SIZE];
    uint32_t head;
    uint32_t tail;
} audio_buffer_t;
