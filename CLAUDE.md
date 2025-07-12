# Claude AI Assistant - Project Documentation

## Project Status: MEMORY OPTIMIZATION + CRITICAL HTTP FIXES COMPLETE ✅
**Last Updated**: August 15, 2025

### Current State: HTTP TIMEOUT & BUFFER FIXES FOR 100KB+ RESPONSES
- **CRITICAL FIX**: HTTP client buffer increased from 4KB to 128KB (prevents memory corruption)
- **CRITICAL FIX**: HTTP timeouts increased from 5s to 15s (prevents connection timeouts)
- **CRITICAL FIX**: Task watchdog timeout increased from 30s to 60s (prevents task reset)
- **WiFi Configuration**: OPTIMIZED for large HTTP transfers (100KB+ responses)
- **SPIRAM Usage**: MAXIMIZED (60%+ utilization achieved)
- **System Status**: STABLE - prevents crashes and timeouts during large HTTP operations
- **Memory Strategy**: All WiFi/networking buffers moved to SPIRAM, HTTP response buffer (128KB) in SPIRAM
- ## CRITICAL FIXES APPLIED - HTTP TIMEOUT AND NETWORKING ISSUES

### August 15, 2025 - COMPLETED

**Issues Resolved:**
1. **Task Watchdog Timeout (30s → 60s)** - Extended timeout for long HTTP operations
2. **HTTP Client Timeout (5s → 15s)** - Increased for large Home Assistant API responses
3. **HTTP Event Post Timeout (2s → 10s)** - Prevents connection timeouts during data transfer
4. **WiFi Configuration Error** - Fixed RX_BA_WIN vs DYNAMIC_RX_BUFFER constraint violation
5. **Memory Watchdog Protection** - Added aggressive watchdog feeding during HTTP operations
6. **Switch Control HTTP POST** - Confirmed working, issue was timeout-related

**Configuration Changes:**
- `CONFIG_ESP_TASK_WDT_TIMEOUT_S=60` (was 30)
- `CONFIG_TASK_WDT_TIMEOUT_S=60` (was 30)
- `CONFIG_ESP_HTTP_CLIENT_EVENT_POST_TIMEOUT=10000` (was 2000)
- `HA_HTTP_TIMEOUT_MS=15000` (was 5000)
- `HA_SYNC_TIMEOUT_MS=15000` (was 5000)
- `CONFIG_ESP_WIFI_RX_BA_WIN=6` (was 16, fixed constraint violation)

**Code Improvements:**
- Enhanced watchdog feeding in HA task manager around HTTP operations
- Added timeout protection for bulk state fetches
- Improved error handling and status reporting for HTTP timeouts
- Better logging for debugging HTTP connection issues

**Switch Control Status:**
✅ Touch switches ARE sending HTTP POST requests
✅ The issue was HTTP connection timeout during large response processing
✅ With extended timeouts, switches now control Home Assistant devices properly
✅ System handles 100KB+ API responses without crashes or timeouts

**Result:** System now reliably processes touch input → HTTP POST → Home Assistant device control

### Critical HTTP Timeout & Buffer Fixes Applied:
🚨 **CRITICAL FIX**: HTTP client buffer increased from 4KB to 128KB
   - `HA_MAX_RESPONSE_SIZE` changed from 4096 to 131072 bytes
   - **Issue**: 4KB buffer caused memory corruption with 100KB+ responses
   - **Solution**: 128KB buffer matches pre-allocated SPIRAM buffer for safe large response handling

🚨 **CRITICAL FIX**: HTTP timeouts extended for large responses
   - `HA_HTTP_TIMEOUT_MS` increased from 5000ms to 15000ms (15 seconds)
   - `HA_SYNC_TIMEOUT_MS` increased from 5000ms to 15000ms (15 seconds)
   - **Issue**: 5s timeout insufficient for 100KB+ downloads from Home Assistant
   - **Solution**: 15s timeout allows reliable large response transfers

🚨 **CRITICAL FIX**: Task watchdog timeout extended for network operations
   - `CONFIG_ESP_TASK_WDT_TIMEOUT_S` and `CONFIG_TASK_WDT_TIMEOUT_S` increased from 30s to 60s
   - `CONFIG_ESP_HTTP_CLIENT_EVENT_POST_TIMEOUT` increased from 2000ms to 10000ms
   - **Issue**: Task watchdog reset during long HTTP operations
   - **Solution**: 60s watchdog timeout + enhanced watchdog feeding prevents spurious resets

🚨 **ENHANCED**: Better watchdog management in HA task
   - Watchdog feeding before and after all HTTP operations
   - Watchdog feeding between individual fallback requests
   - Improved error handling with status reporting to UI

### Validated Configuration Items (Present in sdkconfig):
✅ `CONFIG_SPIRAM_TRY_ALLOCATE_WIFI_LWIP=y` - Forces WiFi/LWIP to use SPIRAM
✅ `CONFIG_ESP_WIFI_DYNAMIC_RX_BUFFER_NUM=20` - Optimized for large HTTP transfers
✅ `CONFIG_ESP_WIFI_RX_BA_WIN=16` - Block ACK window (< DYNAMIC_RX_BUFFER_NUM)
✅ `CONFIG_LV_MEM_SIZE_KILOBYTES=64` - LVGL memory in DRAM (reduced for optimization)

### Removed Invalid/Duplicate Configuration Items:
❌ `CONFIG_LWIP_MEM_SIZE=163840` - Not found in sdkconfig, removed
❌ `CONFIG_SPIRAM_WIFI_CACHE_TX_BUFFER_NUM` - Invalid config removed
❌ `CONFIG_ESP_WIFI_RX_BUFFER_TYPE_SPIRAM` - Redundant with SPIRAM_TRY_ALLOCATE_WIFI_LWIP
❌ `CONFIG_LWIP_MEM_USE_POOLS` - Invalid config removed
❌ `CONFIG_LWIP_MEMP_MEM_MALLOC` - Invalid config removed
❌ `CONFIG_LV_DISP_DEF_REFR_PERIOD` - Invalid LVGL config removed
❌ `CONFIG_ESP_HTTP_CLIENT_RECV_TIMEOUT` - Invalid HTTP client config removed
❌ Duplicate `CONFIG_SPIRAM_TRY_ALLOCATE_WIFI_LWIP=y` - Removed duplicate

### Current Memory Optimization Strategy:
1. **SPIRAM (8MB)**: Maximized for WiFi buffers, HTTP response buffers (128KB)
2. **DRAM**: Task stacks for networking (12KB HA task), LVGL (64KB), essential system components
3. **WiFi Buffers**: 20 dynamic RX buffers in SPIRAM for optimal 100KB+ HTTP performance
4. **HTTP Strategy**: Pre-allocated 128KB buffer in SPIRAM for large API responses
5. **CRITICAL**: Task stacks that make network calls MUST use internal RAM (LWIP requirement)


## Project Overview
- **Project Type**: ESP-IDF RGB LCD Panel Example
- **Target Hardware**: ESP32-S3 (ESP32-8048S050)
- **Display**: 5" IPS LCD with capacitive touch
- **Display Driver**: ST7262
- **LCD Interface**: RGB (16-bit data lines)
- **Graphics Library**: LVGL
- **Display Resolution**: 800x480
- **Touch Interface**: I2C (GT911 capacitive touch controller)
- **Date**: August 13, 2025

## Build and Development Workflow

### VS Code Tasks Integration
This project is configured with VS Code tasks for streamlined development. Use these tasks instead of running ESP-IDF CLI commands directly:

**Available Tasks:**
- `ESP-IDF Build` - Standard build
- `Build with System Manager Debug` - Build with debug configuration
- `ESP-IDF Build Clean` - Clean build
- `ESP-IDF Full Clean Build` - Full clean and rebuild
- `Flash and Monitor with Debug` - Flash firmware and start monitor
- `Build and Flash Debug` - Combined build and flash

**Usage:**
- Press `Ctrl+Shift+P` → `Tasks: Run Task` → Select desired task
- Or use the VS Code task runner interface
- Tasks automatically handle ESP-IDF environment setup

**Advantages:**
- Automatic ESP-IDF environment initialization
- Integrated with VS Code's problem matcher for error highlighting
- Consistent build environment across different terminals
- No need to manually run `export.bat` before each command

## Hardware Configuration
ESP32-8048S050 standard GPIO assignments:
```
LCD RGB Interface:
VSYNC: GPIO41    HSYNC: GPIO39    DE: GPIO40       PCLK: GPIO42
DATA0: GPIO8     DATA1: GPIO3     DATA2: GPIO46    DATA3: GPIO9
DATA4: GPIO1     DATA5: GPIO5     DATA6: GPIO6     DATA7: GPIO7
DATA8: GPIO15    DATA9: GPIO16    DATA10: GPIO4    DATA11: GPIO45
DATA12: GPIO48   DATA13: GPIO47   DATA14: GPIO21   DATA15: GPIO14

Power & Control:
BACKLIGHT: GPIO2 (PWM control)
LCD_RST: Not connected (ST7262 auto-reset)
3V3_LCD: Always on

Touch Interface (GT911):
SDA: GPIO19      SCL: GPIO20      INT: GPIO18      RST: GPIO38

Other Peripherals:
USER_LED: GPIO17    BOOT: GPIO0    USB_D-: GPIO19   USB_D+: GPIO20
```

## Configuration Management

### SDK Configuration Hierarchy
This project follows ESP-IDF's configuration management best practices:

**Configuration Priority (highest to lowest):**
1. `sdkconfig.defaults.esp32s3` - **SINGLE SOURCE OF TRUTH** for ESP32-S3 specific settings
2. `sdkconfig.defaults` - General default configurations
3. `sdkconfig` - Generated file, **DO NOT EDIT MANUALLY**

**Important Rules:**
- ✅ **Always modify** `sdkconfig.defaults.esp32s3` for configuration changes
- ✅ **Reference** `sdkconfig` to search for existing configuration options
- ❌ **Never edit** `sdkconfig` directly - it gets regenerated automatically
- ❌ **Never commit** `sdkconfig` to version control (use `.gitignore`)

### Critical: Configuration Synchronization
⚠️ **ALWAYS delete and regenerate `sdkconfig` when modifying `sdkconfig.defaults.*` files**

ESP-IDF's configuration system can get out of sync, leading to:
- Old configuration values persisting despite defaults changes
- Memory allocation issues (SPIRAM settings not updating)
- Build inconsistencies and runtime failures

**Required Workflow for ANY defaults changes:**
```bash
# Step 1: Delete the generated config file
del sdkconfig

# Step 2: Regenerate with updated defaults
idf.py reconfigure

# Step 3: Verify changes were applied
grep "CONFIG_OPTION_NAME" sdkconfig
```

**Configuration Workflow:**
1. Search for configuration option in `sdkconfig` to find the correct CONFIG name
2. Add or modify the setting in `sdkconfig.defaults.esp32s3`
3. **MANDATORY:** Delete `sdkconfig` file: `del sdkconfig`
4. **MANDATORY:** Regenerate: `idf.py reconfigure`
5. Verify the setting appears correctly in the newly generated `sdkconfig`
6. Build and test to ensure changes are applied

**Example Configuration Categories:**
- Stack sizes: `CONFIG_ESP_MAIN_TASK_STACK_SIZE`, `CONFIG_FREERTOS_*_STACK_SIZE`
- Memory settings: `CONFIG_SPIRAM_*`, heap configurations
- Task priorities and watchdog timeouts
- Hardware peripherals: LCD, WiFi, UART settings
- Build optimizations and debug options

### PSRAM Memory Strategy
This project aggressively utilizes PSRAM for maximum LVGL rendering performance, using 60%+ of the 8MB PSRAM:

**MAXIMUM LVGL PERFORMANCE CONFIGURATION:**
- **Refresh Rate**: Optimized to 10Hz (100ms) - reduces CPU load for memory efficiency
- **SPIRAM_MALLOC_RESERVE_INTERNAL**: Minimal 4KB (from 20KB) to maximize PSRAM usage
- **SPIRAM_MALLOC_ALWAYSINTERNAL**: Minimal 4KB (from 12KB) to prefer PSRAM for all large allocations
- **LVGL Memory Pool**: MASSIVE 5MB (from 128KB) - 62.5% of PSRAM for ultra-smooth UI rendering
- **LVGL Draw Buffer**: 128KB (from 8KB) for immediate-mode rendering performance
- **LVGL Draw Buffer Lines**: Full screen 480 lines for complete screen buffering at 10Hz
- **Pre-allocated HTTP Buffer**: 512KB dedicated buffer in SPIRAM for HA API responses

**Task Priority Optimization for LVGL:**
- **Serial Data Task** (32KB stack, Priority 2): Lower priority to prioritize LVGL rendering
- **WiFi Reconnect Task** (32KB stack, Priority 2): Background process with lower priority
- **Home Assistant Task** (64KB stack, Priority 1): Lowest priority - non-critical timing

**SPIRAM Memory Budget (~5.5MB+ utilized, 70%+ of 8MB):**
- **LVGL Memory Pool**: 5MB (62.5% of PSRAM) - MAXIMUM allocation
- **HTTP Response Buffer**: 512KB dedicated buffer
- **Task Stacks**: 128KB total (Serial: 32KB + WiFi: 32KB + HA: 64KB)
- **LVGL Draw Buffers**: ~768KB (Full 480 lines × 800 pixels × 2 bytes)
- **Available for dynamic allocation**: ~2MB remaining

**Performance Benefits:**
- **Ultra-smooth UI**: 5MB LVGL pool with full-screen buffering at optimized 10Hz
- **Zero allocation overhead**: Pre-allocated 512KB HTTP buffer eliminates malloc/free
- **LVGL rendering priority**: All other tasks run at lower priority for smooth UI
- **Power optimized**: 10Hz refresh reduces power consumption vs 60Hz
- **Future-proof memory**: Massive memory pools handle any UI complexity
- **No memory fragmentation**: Pre-allocated large buffers prevent fragmentation

**Implementation Details:**
- Uses `xTaskCreateStatic()` with `heap_caps_malloc(MALLOC_CAP_SPIRAM)` for stack allocation
- Automatic fallback to standard `xTaskCreatePinnedToCore()` if SPIRAM allocation fails
- Proper cleanup with `heap_caps_free()` when tasks are destroyed
- Task priorities optimized for LVGL rendering performance (lower = higher LVGL priority)
- LVGL draw buffers allocated from SPIRAM for optimal graphics performance
- Main task stack increased to 16KB to handle extensive SPIRAM heap management
- **10Hz Refresh Rate**: Optimized for power efficiency and smooth rendering quality
- **Pre-allocated HTTP Buffer**: 512KB buffer eliminates malloc/free overhead during API calls
- **Total SPIRAM Utilization**: ~5.5MB+ (>70% of 8MB PSRAM) with LVGL maximum priority

### Configuration Troubleshooting
If you experience unexpected behavior after configuration changes:

**Symptoms of configuration desync:**
- Memory allocation failures (e.g., task creation errors)
- WiFi connection issues after buffer size changes
- Boot crashes after SPIRAM setting modifications
- Features not working despite being enabled

**Solution:**
```bash
# Force complete configuration regeneration
del sdkconfig
idf.py fullclean
idf.py reconfigure
idf.py build
```

**Verification Commands:**
```bash
# Check SPIRAM memory settings
grep "CONFIG_SPIRAM_MALLOC_" sdkconfig

# Check WiFi buffer configurations
grep "CONFIG_ESP_WIFI_" sdkconfig

# Check task stack sizes
grep "CONFIG_.*_STACK_SIZE" sdkconfig
```

## Commit Message Convention

Your task is to help the user to generate a commit message and commit the changes using git.

### Guidelines

- DO NOT add any ads such as "Generated with [Claude Code](https://claude.ai/code)"
- Only generate the message for staged files/changes
- Don't add any files using `git add`. The user will decide what to add.
- Follow the rules below for the commit message.

### Format

```
<type>(<scope>): <emoji> <message title>

<bullet points summarizing what was updated>
```

### Rules

* title is lowercase, no period at the end.
* Title should be a clear summary, max 50 characters.
* Use the body (optional) to explain *why*, not just *what*.
* Bullet points should be concise and high-level.

### Avoid

* Vague titles like: "update", "fix stuff"
* Overly long or unfocused titles
* Excessive detail in bullet points

### Allowed Types

| Type     | Description                           |
| -------- | ------------------------------------- |
| feat     | New feature                           |
| fix      | Bug fix                               |
| chore    | Maintenance (e.g., tooling, deps)     |
| docs     | Documentation changes                 |
| refactor | Code restructure (no behavior change) |
| test     | Adding or refactoring tests           |
| style    | Code formatting (no logic change)     |
| perf     | Performance improvements              |

### Emoji Guidelines

Include an emoji in every commit message to categorize the type of change:

| Emoji | Category | Description |
| ----- | -------- | ----------- |
| 📚 | Documentation | Hardware specs, schematics, user guides |
| ⚙️ | Configuration | VS Code settings, build config, toolchain |
| 💄 | Display/Graphics | LCD drivers, LVGL integration, UI components |
| 🔧 | Hardware/Fixes | GPIO config, peripheral drivers, bug fixes |
| ✅ | Testing | Unit tests, integration tests, validation |
| ⬆️ | Dependencies | Component updates, library management |
| ⚡ | Performance | Optimization, memory management, speed |
| 🎨 | UI/UX | Interface design, user experience |
| 🔒 | Security | Authentication, encryption, secure communication |
| 📱 | Touch/Mobile | Touch interfaces, gesture handling |

### Example Titles

```
feat(auth): 🔑 add JWT login flow
fix(ui): 🐛 handle null pointer in sidebar
refactor(api): ♻️ split user controller logic
docs(readme): 📝 add usage section
```

### Example with Title and Body

```
feat(auth): 🔑 add JWT login flow

- Implemented JWT token validation logic
- Added documentation for the validation component
```

---
*This documentation provides project overview and commit message guidelines for the ESP-IDF RGB LCD Panel project.*
