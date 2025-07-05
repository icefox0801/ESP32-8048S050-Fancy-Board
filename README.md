# ESP32-S3 Smart Home Dashboard

A comprehensive system monitor and smart home control dashboard for ESP32-S3-8048S050 with 5.0" RGB LCD display.

## Features

- **Real-time System Monitoring**: CPU, GPU, and memory usage display
- **Smart Home Controls**: Aquarium automation (water pump, wave maker, light control, feeding)
- **Touch Interface**: Capacitive touch with GT911 controller
- **WiFi Connectivity**: Auto-reconnect with status monitoring
- **Home Assistant Integration**: REST API client for IoT device control
- **Professional UI**: Multi-panel dashboard with LVGL graphics

## Hardware Support

- **Display**: ESP32-S3-8048S050 (800×480 RGB565 LCD)
- **Touch**: GT911 capacitive touch controller
- **Memory**: 8MB PSRAM for framebuffer allocation
- **Connectivity**: WiFi with automatic reconnection

## Quick Start

### 1. Hardware Setup
Connect your ESP32-S3-8048S050 development board via USB for power and programming.

### 2. Configuration Setup
**WiFi & Home Assistant Setup (Required):**
```bash
# Copy and edit WiFi credentials
cp main/wifi/wifi_config_template.h main/wifi/wifi_config.h

# Copy and edit Home Assistant settings
cp main/smart/smart_config_template.h main/smart/smart_config.h
```
**Get HA Token:** Profile → Long-Lived Access Tokens → Create "ESP32 System Monitor" → Add to smart_config.h

### 3. Build and Flash
```bash
idf.py build flash monitor
```

## Architecture

- **Main App**: System initialization and task coordination
- **LVGL Setup**: Display driver with PSRAM framebuffers
- **Touch Driver**: GT911 I2C communication with calibration
- **WiFi Manager**: Auto-connect with retry logic
- **Smart Home API**: HTTP client for Home Assistant REST API
- **UI Components**: Multi-panel dashboard with real-time updates

## Display Layout

- **Top Panel**: Smart home controls (Water Pump, Wave Maker, Light, Feed Button)
- **Middle Panels**: CPU and GPU monitoring with temperature and usage
- **Memory Panel**: System memory usage with progress bar
- **Status Bar**: Serial connection and WiFi status

## Troubleshooting

- **Display Issues**: Check PCLK frequency and RGB timing parameters
- **Touch Not Working**: Verify GT911 I2C connections (SDA: GPIO19, SCL: GPIO20)
- **WiFi Connection**: Check credentials in `wifi_config.h`
- **Memory Errors**: Enable PSRAM in menuconfig for framebuffer allocation

## Security Notes

- Configuration files `wifi_config.h` and `smart_config.h` are git-ignored
- Never commit sensitive credentials to version control
- Use template files as configuration reference
