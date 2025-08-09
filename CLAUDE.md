# Claude AI Assistant - Project Documentation

## Project Overview
**ESP32-S3 Home Assistant Dashboard with 5" Touch Display**

- **Hardware**: ESP32-S3-8048S050 (5" IPS LCD + Capacitive Touch)
- **Display**: 800x480 RGB LCD (ST7262 driver)
- **Graphics**: LVGL with optimized SPIRAM memory management
- **Connectivity**: WiFi + Home Assistant REST API integration
- **Features**: Touch-controlled smart home switches, system monitoring
- **Last Updated**: August 20, 2025

## Quick Start

### VS Code Tasks (Recommended)
Use VS Code tasks for streamlined development:

1. **Build**: `Ctrl+Shift+P` → `Tasks: Run Task` → `ESP-IDF Build`
2. **Flash**: `ESP-IDF Flash` (⚠️ Stop monitor first!)
3. **Monitor**: `ESP-IDF Monitor`
4. **Clean**: `ESP-IDF Full Clean`

**Critical Workflow:**
1. Stop any running monitor tasks before flashing
2. Build → Flash → Monitor
3. If config changed: Delete `sdkconfig` → Run `ESP-IDF Reconfigure`

## Hardware Configuration

### Essential GPIO Mapping
```c
// LCD RGB Interface (ST7262)
VSYNC: GPIO41    HSYNC: GPIO39    DE: GPIO40       PCLK: GPIO42
DATA0-15: GPIO8,3,46,9,1,5,6,7,15,16,4,45,48,47,21,14
BACKLIGHT: GPIO2 (PWM)

// Touch Interface (GT911)
SDA: GPIO19      SCL: GPIO20      INT: GPIO18      RST: GPIO38

// System
USER_LED: GPIO17    BOOT: GPIO0
```

## Code Standards & Logging Patterns

### Centralized Logging System
Use `system_debug_utils.h` for all logging:

```c
#include "system_debug_utils.h"

// Standard functions
debug_log_startup(DEBUG_TAG_*, "Component");
debug_log_info(DEBUG_TAG_*, "Message");
debug_log_error_f(DEBUG_TAG_*, "Error: %s", msg);

// Available tags: DEBUG_TAG_MAIN, DEBUG_TAG_WIFI, DEBUG_TAG_HA,
// DEBUG_TAG_UI, DEBUG_TAG_TOUCH, DEBUG_TAG_PARSER, DEBUG_TAG_SERIAL
```

**Best Practices:**
- ✅ Use `debug_log_*()` functions only
- ❌ Avoid `printf()`, `ESP_LOG*()`, emojis
- ❌ Remove verbose logging in performance-critical loops
- ✅ Keep essential error/warning messages

## Configuration Management

### SDK Configuration
**Single source of truth**: `sdkconfig.defaults.esp32s3`

**Process**:
1. Edit sdkconfig.defaults.esp32s3
2. Delete sdkconfig → Run "ESP-IDF Reconfigure"
3. Build → Flash → Monitor

**Never edit sdkconfig directly** - it's auto-generated.

### SPIRAM Strategy (5MB LVGL Pool)
```bash
CONFIG_SPIRAM=y
CONFIG_LV_MEM_SIZE=5242880  # 5MB for smooth UI
```

**Memory Split**:
- **SPIRAM**: LVGL buffers, graphics data (5MB+)
- **DRAM**: System tasks, networking, real-time operations

## Commit Messages
+ **Types**: feat, fix, refactor, docs, config
+ **Git Emojis**: ✨ feat, 🐛 fix, ♻️ refactor, 📝 docs, ⚙️ config, 🚀 perf, 💄 ui, 🔧 hardware
+ **No commit body required - keep messages concise in title only**

### Format
```
<type>(<scope>): <emoji> <message title>
```


### Examples
```
feat: ✨ optimize LVGL memory allocation
fix: 🐛 resolve touch calibration drift
docs: 📝 update GPIO mapping
config: ⚙️ increase SPIRAM pool size
refactor: ♻️ simplify HTTP client logic
```

---
*Last updated: August 20, 2025*
