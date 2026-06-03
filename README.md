# Dead Reckoning

> *To navigate by dead reckoning is to estimate your position using a known starting point, elapsed time, and direction — without external reference.*

A Critical Engineering project for the **Heltec WiFi LoRa 32 V3**.

Dead Reckoning runs three concurrent tasks on the ESP32-S3:

- **Core 0 — LoRa mesh**: listens on 868 MHz LoRa for packets from other nodes and relays them onward. This node does not originate transmissions — it is a witness and a relay, not a sender.
- **Core 1 — probe sniffer**: passively captures 802.11 probe request frames — the involuntary broadcasts every smartphone emits when scanning for known WiFi networks. MACs are SHA-256 hashed; raw addresses are never stored.
- **Display**: the built-in 0.96" OLED rotates every 5 seconds between mesh stats, probe stats, and a project statement.

---

## Install in one click

**[ZeroOneTwo-CreativeTechnologyHub.github.io/dead-reckoning](https://ZeroOneTwo-CreativeTechnologyHub.github.io/dead-reckoning)**

Connect your Heltec V3 via USB-C and press the Install button. Chrome or Edge required. No drivers, no IDE.

---

## Build from source

```bash
git clone https://github.com/ZeroOneTwo-CreativeTechnologyHub/dead-reckoning
cd dead-reckoning/firmware
pip install platformio
pio run
```

Firmware compiles with PlatformIO. The post-build script merges the binary for ESP Web Tools and writes it to `docs/assets/firmware.bin`.

---

## Repository structure

```
dead-reckoning/
├── firmware/
│   ├── platformio.ini         # PlatformIO project config
│   ├── scripts/
│   │   └── merge_firmware.py  # Merges .bin for ESP Web Tools
│   └── src/
│       ├── main.cpp            # Entry point, task spawning
│       ├── shared_state.h/cpp  # Mutex-guarded shared data
│       ├── display.h/cpp       # OLED screen cycling
│       ├── lora_mesh.h/cpp     # LoRa receive/relay task
│       └── probe_sniffer.h/cpp # WiFi probe capture task
├── docs/                       # GitHub Pages site
│   ├── index.html              # Installer and project page
│   └── manifest.json           # ESP Web Tools manifest
└── .github/
    └── workflows/
        └── build-deploy.yml    # CI: compile and deploy Pages
```

---

## Hardware

| Component | Notes |
|---|---|
| Heltec WiFi LoRa 32 V3 | 868 MHz for UK/EU. 915 MHz for US. |
| 868 MHz antenna | Usually included. SMA stub or whip. |
| USB-C power | Powerbank, mains adapter, or laptop. |
| LiPo 3.7V 1000mAh or greater (optional) | Uses the Heltec's onboard BMS. |

---

## Privacy and legal

**Probe capture (UK legal basis):**

The sniffer captures unencrypted 802.11 management frames broadcast publicly by nearby devices. These are not private communications and are not protected by the Computer Misuse Act 1990, which concerns unauthorised access to computer material, not passive radio reception of broadcast frames.

**MAC address handling:**

Raw MAC addresses are never stored, logged, or transmitted. Each source MAC is immediately passed through SHA-256 and only the first 8 bytes of the digest are retained as a fingerprint. This is consistent with ICO guidance that hashed pseudonymous identifiers, where the original data is not retained, fall outside the definition of personal data under UK GDPR.

**SSID strings:**

Probe requests contain the SSID (network name) the device is actively seeking. These are strings the device is broadcasting publicly and are not personal data.

**What this project does not do:**

- Store, log, or transmit raw MAC addresses
- Capture WiFi data frames (only management frames)
- Perform packet injection or deauthentication attacks
- Identify or track specific individuals

---

## Critical Engineering alignment

This project is built in response to the [Critical Engineering Manifesto](https://criticalengineering.org) by Julian Oliver, Gordan Savičić, and Danja Vasiliev.

| Principle | Application |
|---|---|
| 1 — technology as challenge and threat | The WiFi/cellular stack surrounding us is studied, not merely consumed |
| 5 — engineering engineers its user | Each probe request logged is evidence of a device being engineered by infrastructure it did not choose |
| 7 — space between production and consumption | The moment of the probe broadcast is that space; Dead Reckoning acts there |
| 10 — the exploit as exposure | The network was always watching. This device watches back. |

---

## Contributing

Pull requests welcome. Open an issue before large changes.

Please keep the privacy commitments (no raw MACs, no data exfiltration) intact in any fork or derivative work.

---

## Licence

GNU General Public License v3.0 — see [LICENSE](LICENSE)
