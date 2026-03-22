# ESPHome NTP Server Component

A Stratum-1 NTP server external component for ESPHome. Turns your ESP8266 or ESP32 into a local network time server backed by any ESPHome time source (GPS, SNTP, Home Assistant, etc.).

Implements a lightweight NTPv4 server (RFC 5905) that listens for client requests on UDP port 123 and responds with the current time. Identifies itself as Stratum 1 with reference ID `GPS`.

## Features

- Stratum-1 NTPv4 server on configurable UDP port (default 123)
- Works with any ESPHome `time` platform (GPS, SNTP, Home Assistant)
- Responds only when the time source has a valid fix — silently ignores requests otherwise
- Compatible with ESP8266 and ESP32
- Standard NTP clients (ntpdate, chrony, systemd-timesyncd, Windows w32tm) work out of the box

## Installation

### Option 1: From GitHub

Add to your ESPHome YAML:

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/JaccovL/esphome-ntp-server
      ref: main
    components: [ntp_server]
```

### Option 2: Local

Copy the `ntp_server/` folder into a `my_components/` directory alongside your YAML config:

```
/config/esphome/
├── your_device.yaml
└── my_components/
    └── ntp_server/
        ├── __init__.py
        ├── ntp_server.h
        └── ntp_server.cpp
```

Then reference it in your YAML:

```yaml
external_components:
  - source:
      type: local
      path: my_components
    components: [ntp_server]
```

## Configuration

```yaml
ntp_server:
  time_id: my_time_source
  port: 123  # optional, default is 123
```

### Parameters

| Parameter | Required | Default | Description |
|-----------|----------|---------|-------------|
| `time_id` | Yes | — | ID of an ESPHome `time` component to use as the clock source |
| `port` | No | `123` | UDP port to listen on |

## Example: GPS Time Server

This example uses a GPS module connected via UART as the time source and serves that time over NTP to the local network.

```yaml
uart:
  rx_pin: GPIO13
  tx_pin: GPIO15
  baud_rate: 9600

gps:
  latitude:
    name: "Latitude"
  longitude:
    name: "Longitude"

time:
  - platform: gps
    id: gps_time
    timezone: "Asia/Kolkata"

ntp_server:
  time_id: gps_time
  port: 123
```

## Example: Home Assistant Time Relay

You can also relay Home Assistant's time to devices that can't reach the internet:

```yaml
time:
  - platform: homeassistant
    id: ha_time

ntp_server:
  time_id: ha_time
```

## Testing

Once flashed and connected, verify the NTP server is working:

**Linux / macOS:**
```bash
ntpdate -q <device-ip>

# or
sntp <device-ip>
```

**Windows:**
```powershell
w32tm /stripchart /computer:<device-ip> /dataonly /samples:3
```

**Other ESPHome devices:**
```yaml
time:
  - platform: sntp
    servers:
      - <device-ip>
      - pool.ntp.org  # fallback
```

## Configuring Clients

### Linux (systemd-timesyncd)

Edit `/etc/systemd/timesyncd.conf`:

```ini
[Time]
NTP=<device-ip>
FallbackNTP=pool.ntp.org
```

Restart: `sudo systemctl restart systemd-timesyncd`

### Linux (chrony)

Add to `/etc/chrony/chrony.conf`:

```
server <device-ip> iburst prefer
```

Restart: `sudo systemctl restart chrony`

### Windows

```powershell
w32tm /config /manualpeerlist:"<device-ip>" /syncfromflags:manual /update
net stop w32time && net start w32time
```

## How It Works

The component opens a UDP socket on the configured port during `setup()`. On every `loop()` iteration it checks for incoming NTP request packets (48 bytes). When a valid request arrives and the time source has a fix, it constructs a standard NTPv4 response:

- **LI** (Leap Indicator): 0 (no warning)
- **Version**: 4 (NTPv4)
- **Mode**: 4 (server)
- **Stratum**: 1 (primary reference)
- **Reference ID**: `GPS\0`
- **Precision**: -20 (~microsecond)

The response includes reference, receive, and transmit timestamps derived from the current time source, plus the client's original transmit timestamp copied into the origin field for request-response matching.

## Limitations

- Accuracy is typically within +/-1 second, limited by the ESP's processing loop rather than the GPS clock/source itself. This is more than sufficient for home/IoT use cases.
- The NTP server only responds after the time source reports a valid time. During GPS cold start (1-5 minutes), requests are silently dropped.
- Sub-second (fractional) NTP timestamps are always zero. Clients will still sync correctly but won't achieve sub-second precision.
- Single-threaded UDP handling — suitable for a handful of clients polling every few minutes, not for high-traffic use.

## License

MIT
