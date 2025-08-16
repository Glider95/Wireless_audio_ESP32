
# ESP32-C5 Wi-Fi 6 Multichannel Audio System

This project implements a wireless multichannel audio link using **ESP32-C5 boards**. It captures multichannel PCM (I²S) audio and distributes it wirelessly to multiple receivers over 5 GHz Wi-Fi 6 with low latency. The design is inspired by commercial wireless audio systems, but is fully open and DIY.

## Project Overview

The system consists of three main components:

1. **Ingest Nodes (4× ESP32-C5)**
   - Each captures one I²S data line (SD0..SD3) from a multichannel audio source.
   - Operates as I²S slave (using shared BCLK/LRCLK).
   - Sends 1 ms stereo PCM frames via UDP unicast to the Aggregator.

2. **Aggregator (1× ESP32-C5 in AP mode)**
   - Collects the 4 stereo streams.
   - Re-timestamps and re-sequences frames.
   - Re-broadcasts them over multicast groups (one per stereo pair).

3. **Receivers (1+ ESP32-C5 per speaker pair)**
   - Each subscribes to the multicast group for its assigned channel (Front L/R, Center/Sub, Surround, Rear).
   - Buffers for jitter and plays out via I²S master to a DAC/amp.

## Hardware Setup

- **Ingest ESP32-C5 boards (x4):**
  - Connect BCLK, LRCLK, and one SDx line (0..3) from your audio source.
  - Each board captures a stereo pair.
- **Aggregator ESP32-C5:**
  - Runs as Wi-Fi 6 AP (default SSID: `WiSA-DIY`, channel 36, WPA2).
  - Collects all 4 streams and multicasts to receivers.
- **Receiver ESP32-C5 boards (x4 or more):**
  - Connect to Aggregator AP.
  - Subscribe to one of the multicast groups (e.g., `239.255.0.11` for Front L/R).
  - I²S Master out to DAC/amp.

## Project Structure

```
common_audio.h    # Shared packet header definition
ingest/           # I²S → UDP unicast (4 builds: SD0..SD3)
aggregator/       # Collects unicast, re-multicasts to 4 groups
receiver/         # Subscribes to one group, plays I²S out
```

## Packet Format

Every 1 ms frame sent over UDP:

```
[Header 16 bytes +]
  magic:       "AUD0"
  seq_num:     Global sequence number (monotonic)
  sample_ctr:  Total samples since boot (per lane)
  timestamp:   Capture time in µs
  sd_index:    Which SDx lane (0..3)
  sample_bytes:3 (24-bit samples packed in 32-bit slots)
  num_channels:2 (stereo)
[Payload]
  PCM audio frames, stereo interleaved
```

## Build & Flash

- Requires **ESP-IDF 5.x** (with ESP32-C5 support)
- For each sub-project (`ingest/`, `aggregator/`, `receiver/`):

```bash
idf.py set-target esp32c5
idf.py menuconfig   # adjust pins if needed
idf.py build
idf.py flash monitor
```

- For ingest builds, set `SD_INDEX` = 0,1,2,3 (in `main.c`)

## How to Deploy

1. Flash 4 Ingest nodes, each on its own SD line (SD0..SD3).
2. Flash 1 Aggregator (becomes AP, SSID `WiSA-DIY`).
3. Flash 4 Receivers, each pointed to one multicast IP.
4. Connect your DACs/amps to each Receiver’s I²S out.
5. Play audio into your multichannel source → multichannel wireless output!

## Notes

- This project is inspired by commercial wireless audio systems (such as WiSA), but is not affiliated with or certified by any such standard.
- Latency and jitter depend on your RF environment and network setup.
- Use for experimentation, DIY audio, or learning.
```
