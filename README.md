Got it 👍 — here’s a **README.md** that documents everything we discussed, in a way that you (or anyone else) could drop into a Git repo for this ESP32-C5 “mini-WiSA” project.

---

# README.md

# 🎶 ESP32-C5 Wi-Fi 6 Multichannel Audio System (“Mini-WiSA”)

This project implements a **DIY WiSA-like wireless audio link** using **ESP32-C5 boards**.
It captures **multichannel PCM (I²S)** from a WiSA SoundSend board (or any HDMI/ARC audio DSP), then distributes it wirelessly to multiple receivers over **5 GHz Wi-Fi 6** with low latency.

---

## 🚀 Project Overview

Commercial WiSA gear uses proprietary 5 GHz RF for **8-channel PCM @ 24-bit / 48 kHz** with \~5 ms latency.
Our ESP32-C5 design mimics the architecture:

1. **Ingest Nodes (4× ESP32-C5)**

   * Each taps one **I²S data line (SD0..SD3)** from the SoundSend’s WiSA header.
   * Operates as **I²S slave** (using SoundSend’s BCLK/LRCLK).
   * Sends 1 ms stereo PCM frames via **UDP unicast** to the Aggregator.

2. **Aggregator (1× ESP32-C5 in AP mode)**

   * Collects the 4 stereo streams.
   * Re-timestamps and re-sequences frames.
   * Re-broadcasts them over **multicast groups** (one per stereo pair).

3. **Receivers (1+ ESP32-C5 per speaker pair)**

   * Each subscribes to the multicast for its assigned channel (Front L/R, C+Sub, Surround, Rear).
   * Buffers \~12 ms for jitter.
   * Plays out via **I²S master** to a DAC/amp.

---

## 📡 Why ESP32-C5?

* **Dual-band Wi-Fi 6** (802.11ax, 2.4 GHz + 5 GHz)
* **OFDMA + MU-MIMO** → better latency and parallel streams
* **I²S peripheral** (slave or master mode)
* **240 MHz RISC-V core** with DMA for real-time audio

This allows us to stream **8 channels at <20 ms latency**, phase-aligned across receivers.

---

## 🔌 Hardware Setup

### SoundSend WiSA header signals:

* **I2S\_SCLK** → BCLK (\~3.072 MHz)
* **I2S\_LRCLK** → LRCLK (48 kHz)
* **I2S\_SD0..SD3** → 4× stereo data lanes (Front, Center/Sub, Surround, Rear)
* **I2S\_MCLK** (24.576 MHz) → not required for ESP32 capture
* **3.3 V & GND**

### Ingest ESP32-C5 boards (x4):

* `BCLK` ← I2S\_SCLK
* `LRCLK` ← I2S\_LRCLK
* `DIN`   ← I2S\_SDx (0..3, one per board)
* `GND` common
* Powered separately (don’t draw from SoundSend’s 3.3 V rail)

### Aggregator ESP32-C5:

* Runs as **Wi-Fi 6 AP** (`SSID=WiSA-DIY`, channel 36, WPA2).
* Collects all 4 streams and multicasts to receivers.

### Receiver ESP32-C5 boards (x4 or more):

* Connect to Aggregator AP.
* Subscribe to one of:

  * `239.255.0.11` (SD0 → Front L/R)
  * `239.255.0.12` (SD1 → Center/Sub)
  * `239.255.0.13` (SD2 → Surround L/R)
  * `239.255.0.14` (SD3 → Rear L/R)
* I²S Master out to DAC/amp:

  * `BCLK`, `LRCLK`, `DOUT`

---

## 🗂 Project Structure

```
/mini-wisa/
├── common_audio.h    # Shared packet header definition
├── ingest/           # I²S → UDP unicast (4 builds: SD0..SD3)
├── aggregator/       # Collects unicast, re-multicasts to 4 groups
└── receiver/         # Subscribes to one group, plays I²S out
```

---

## 📦 Packet Format

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

---

## ⏱ Latency Budget

| Stage                       | Time (ms) |
| --------------------------- | --------- |
| Ingest I²S buffer           | 1–2       |
| Wi-Fi unicast (Ingest→Aggr) | 2–4       |
| Aggregator buffering        | 1–2       |
| Wi-Fi multicast (Aggr→RX)   | 2–4       |
| Receiver buffer             | 8–12      |
| **Total**                   | **11–18** |

✅ Stable for lip-sync & surround use
⚠️ Higher than real WiSA (\~5 ms), but achievable with open ESP32 hardware

---

## 🛠 Build & Flash

* Requires **ESP-IDF 5.x** (with ESP32-C5 support)
* For each sub-project (`ingest/`, `aggregator/`, `receiver/`):

```bash
idf.py set-target esp32c5
idf.py menuconfig   # adjust pins if needed
idf.py build
idf.py flash monitor
```

* For ingest builds, set `SD_INDEX` = 0,1,2,3 (in `main.c`)

---

## 🎛 How to Deploy

1. Flash **4 Ingest nodes**, each on its own SD line (SD0..SD3).
2. Flash **1 Aggregator** (becomes AP, SSID `WiSA-DIY`).
3. Flash **4 Receivers**, each pointed to one multicast IP.
4. Connect your DACs/amps to each Receiver’s I²S out.
5. Play audio into SoundSend → multichannel wireless output!

---

## 🧭 Future Improvements

* **Jitter guard** at aggregator (wait until all 4 lanes present before re-broadcast)
* **PTP-lite sync** between aggregator and receivers to trim drift
* **FEC** (forward error correction) for burst packet loss
* **Bit-packing** 24-bit samples to reduce bandwidth \~25%
* **Wi-Fi 6e (6 GHz)** when ESP32-C6 successor supports it

---

## ⚠️ Disclaimer

* This is **not WiSA-certified**.
* Latency/jitter depend on your RF environment.
* Use for experimentation, DIY audio, or learning.

---

👉 This README explains the full design, wiring, code roles, and usage of the **ESP32-C5 Wi-Fi 6 multichannel audio link**, derived from our deep-dive on SoundSend’s I²S signals and WiSA architecture.

---

Would you like me to also include **diagrams (block & wiring)** in the README (as ASCII or SVG) so the setup is crystal-clear for someone opening the repo cold?
