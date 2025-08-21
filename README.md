# ESP32-S3 Smart Home Dashboard

A comprehensive system monitor and smart home control dashboard for ESP32-S3-8048S050 with 5.0" RGB LCD display.

![IMG_8007_compressed](https://github.com/user-attachments/assets/8db2c69f-94ac-45ce-9d49-30eb6b1fe09f)

## Features

- **Real-time System Monitoring**: CPU, GPU, and memory usage display
- **Smart Home Controls**: Aquarium automation (water pump, wave maker, light control, feeding)
- **Touch Interface**: Capacitive touch with GT911 controller
- **WiFi Connectivity**: Auto-reconnect with status monitoring
- **Home Assistant Integration**: REST API client for IoT device control
- **Professional UI**: Multi-panel dashboard with LVGL graphics

## 🖥️ System Performance Integration

This dashboard integrates with **[SystemPerformanceNotifierService](https://github.com/icefox0801/SystemPerformanceNotifierService)** - A Windows background service that collects and transmits real-time system performance data (CPU, GPU, memory usage) via serial connection to the ESP32 dashboard.

## 💻 Tech Stack

### Hardware
- **Board**: ESP32-S3-8048S050 (Waveshare)
- **Display**: 5.0" IPS LCD (800×480, RGB565)
- **Touch**: GT911 capacitive controller
- **Memory**: 8MB PSRAM, 512KB SRAM
- **Storage**: 16MB Flash
- **Connectivity**: WiFi 802.11 b/g/n

### Software
- **Framework**: ESP-IDF v5.5
- **Graphics**: LVGL v9.2.0
- **Language**: C17
- **Build System**: CMake + Ninja
- **IDE**: VS Code with ESP-IDF extension

## 🚀 Quick Start

### Prerequisites
- **ESP-IDF v5.5** installed and configured
- **VS Code** with ESP-IDF extension
- **Git** for version control

### 1. Clone and Setup
```bash
git clone <repository-url>
cd ESP32-8048S050-Fancy-Board
```

### 2. Configuration
```bash
# Configure WiFi credentials
cp main/wifi/wifi_config_template.h main/wifi/wifi_config.h
# Edit wifi_config.h with your network details

# Configure Home Assistant
cp main/smart/smart_config_template.h main/smart/smart_config.h
# Edit smart_config.h with your HA URL and token
```

### 3. Build and Flash
```bash
# Build project
idf.py build

# Flash to device (ensure ESP32 is connected)
idf.py flash

# Monitor output
idf.py monitor
```

## 🔧 Development Scripts

### VS Code Tasks (Recommended)
- **Build**: `Ctrl+Shift+P` → `Tasks: Run Task` → `ESP-IDF Build`
- **Flash**: `ESP-IDF Flash`
- **Monitor**: `ESP-IDF Monitor`
- **Clean**: `ESP-IDF Full Clean`

### Command Line
```bash
# Full development cycle
idf.py build flash monitor

# Clean rebuild
idf.py fullclean
idf.py build

# Configuration menu
idf.py menuconfig
```

## 🏗️ Architecture

### Core Components
- **Main App**: System initialization and task coordination
- **Display Driver**: LVGL with PSRAM framebuffers
- **Touch Interface**: GT911 I2C with calibration
- **WiFi Manager**: Auto-connect with retry logic
- **Smart Home API**: HTTP client for Home Assistant
- **Serial Monitor**: System performance data reception

### UI Layout
- **Top Panel**: Smart home controls (Water Pump, Wave Maker, Light, Feed)
- **CPU/GPU Panels**: Real-time monitoring with temperature and usage
- **Memory Panel**: System memory usage with progress indicators
- **Status Bar**: Connection status, runtime, and system info

## 🛠️ Hardware Pinout

| Component | Pin | Function |
|-----------|-----|----------|
| **Display** | GPIO2 | Backlight PWM |
| | GPIO8-21 | RGB Data Bus |
| | GPIO39-42 | Control Signals |
| **Touch** | GPIO19/20 | I2C SDA/SCL |
| | GPIO18/38 | INT/RST |
| **System** | GPIO17 | User LED |
| | GPIO0 | Boot Button |

## 📚 Documentation

- **[CLAUDE.md](CLAUDE.md)** - Development workflows, code patterns, and technical details
- **[Home Assistant Setup Guide](docs/ha-setup.md)** - Complete HA integration guide
- **[Hardware Docs](5.0inch_ESP32-8048S050/)** - Official hardware documentation

## 🔍 Troubleshooting

### Common Issues
- **Build fails**: Check ESP-IDF version (requires v5.5)
- **Flash fails**: Ensure correct USB port and driver installation
- **Display blank**: Verify hardware connections and power supply
- **Touch not working**: Check I2C connections and calibration
- **WiFi issues**: Verify credentials in `wifi_config.h`

### Debug Tools
- Serial monitor for system logs
- Built-in crash log manager
- Memory usage monitoring
- Connection status indicators

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Troubleshooting

- **Display Issues**: Check PCLK frequency and RGB timing parameters
- **Touch Not Working**: Verify GT911 I2C connections (SDA: GPIO19, SCL: GPIO20)
- **WiFi Connection**: Check credentials in `wifi_config.h`
- **Memory Errors**: Enable PSRAM in menuconfig for framebuffer allocation
- **System Crashes**: Check crash logs at startup for debugging information

## Security Notes

- Configuration files `wifi_config.h` and `smart_config.h` are git-ignored
- Never commit sensitive credentials to version control
- Use template files as configuration reference


