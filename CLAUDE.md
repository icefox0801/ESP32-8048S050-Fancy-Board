# Claude AI Assistant - Development Documentation

## Project Context
**ESP32-S3 Home Assistant Dashboard with 5" Touch Display**

- **Hardware**: ESP32-S3-8048S050 (5" IPS LCD + Capacitive Touch)
- **Display**: 800x480 RGB LCD (ST7262 driver)
- **Graphics**: LVGL v9.2.0 with optimized SPIRAM memory management
- **Connectivity**: WiFi + Home Assistant REST API integration
- **Features**: Touch-controlled smart home switches, system monitoring
- **Last Updated**: August 21, 2025

## Development Workflows

### Primary Development Cycle
**Standard Workflow:**
1. **Code Changes** → Edit source files
2. **Build** → `ESP-IDF Build` task or `idf.py build`
3. **Flash** → `ESP-IDF Flash` task (⚠️ Stop monitor first!)
4. **Monitor** → `ESP-IDF Monitor` task to view output
5. **Test** → Verify functionality on hardware
6. **Commit** → `git add -A && git commit -m "type: 🔸 message"`

### VS Code Integration (Recommended)
**Essential Tasks:**
- `ESP-IDF Build` - Compile project with full dependency checking
- `ESP-IDF Flash` - Upload firmware to device
- `ESP-IDF Monitor` - Real-time serial output monitoring
- `ESP-IDF Full Clean` - Complete rebuild when needed

**Critical Rules:**
- Always stop monitor tasks before flashing (port conflicts)
- Use tasks instead of command line for consistency
- Check build warnings before flashing
- Monitor immediately after flash to catch startup issues

### Configuration Management Workflow
**Best Practices:**
1. **Single Source**: Only edit `sdkconfig.defaults.esp32s3`
2. **Clean Regeneration**: Delete `sdkconfig` → Run `ESP-IDF Reconfigure`
3. **Verification**: Build and test after configuration changes
4. **Version Control**: Commit `sdkconfig.defaults.esp32s3` changes

**Never edit `sdkconfig` directly** - it's auto-generated and will be overwritten.

## Code Architecture & Patterns

### Component Organization Philosophy
**Modular Design Principles:**
- **Single Responsibility**: Each component handles one specific domain
- **Clean Interfaces**: Public APIs with clear input/output contracts
- **Dependency Injection**: Components register callbacks rather than direct coupling
- **Error Propagation**: Consistent error handling with ESP-IDF error codes
- **Resource Management**: Clear initialization/cleanup patterns

### Project Structure Logic
```
main/
├── dashboard_main.c         # Application orchestration
├── ui/                      # LVGL interface components
├── serial/                  # Data communication layer
├── wifi/                    # Network connectivity
├── smart/                   # Home Assistant integration
├── touch/                   # Hardware input interface
└── utils/                   # Shared system utilities
```

### Component Lifecycle Pattern
**Standard Component Interface:**
1. **Init Function**: `component_init()` - Setup hardware/resources
2. **Start Function**: `component_start()` - Begin operations/tasks
3. **Stop Function**: `component_stop()` - Graceful shutdown
4. **Callback Registration**: `component_register_callback()` - Event handling
5. **Status Queries**: `component_get_status()` - State inspection

### Memory Management Strategy
**SPIRAM vs DRAM Allocation:**
- **SPIRAM**: LVGL framebuffers, large data structures, graphics assets
- **DRAM**: Real-time tasks, interrupt handlers, small frequent allocations
- **Safety**: Always check allocation success before use
- **Cleanup**: Proper resource deallocation in error paths

## Logging & Debug Practices

### Centralized Logging Architecture
**System Philosophy:**
- **Unified Interface**: Single `system_debug_utils.h` for all logging
- **Contextual Tags**: Component-specific debug tags for filtering
- **Severity Levels**: Appropriate log levels for different message types
- **Performance Aware**: Minimal overhead in release builds

**Available Debug Tags:**
`DEBUG_TAG_MAIN`, `DEBUG_TAG_WIFI`, `DEBUG_TAG_HA`, `DEBUG_TAG_UI`, `DEBUG_TAG_TOUCH`, `DEBUG_TAG_PARSER`, `DEBUG_TAG_SERIAL_DATA`, `DEBUG_TAG_SYSTEM`

**Logging Best Practices:**
- Use appropriate severity levels (startup, info, warning, error, debug)
- Include context and actionable information in messages
- Use formatted versions for dynamic data
- Avoid logging in performance-critical sections
- Remove verbose debug logs before production

### Error Handling Patterns
**Defensive Programming:**
- **Null Pointer Checks**: Validate all input parameters
- **Resource Validation**: Check allocation success before use
- **Boundary Checks**: Prevent buffer overflows and array bounds violations
- **Division Safety**: Always check divisors for zero before division
- **ESP-IDF Integration**: Use standard ESP error codes and handling

## Configuration Management

### SDK Configuration Strategy
**Configuration Hierarchy:**
- **Base**: ESP-IDF defaults for ESP32-S3
- **Project**: `sdkconfig.defaults.esp32s3` (version controlled)
- **Generated**: `sdkconfig` (auto-generated, not committed)

**Best Practices:**
- Only edit `sdkconfig.defaults.esp32s3`
- Delete `sdkconfig` after changing defaults
- Run `ESP-IDF Reconfigure` to regenerate
- Test build after configuration changes
- Commit defaults file with descriptive messages

### SPIRAM Memory Strategy
**Allocation Philosophy:**
- **5MB LVGL Pool**: Dedicated graphics memory for smooth UI
- **SPIRAM Priority**: Large buffers, graphics assets, non-real-time data
- **DRAM Reserve**: Critical system tasks, interrupt handlers, networking
- **Performance Balance**: Optimize for UI responsiveness while maintaining system stability

## Debugging & Testing Workflows

### Crash Analysis System
**Comprehensive Crash Management:**
- **Automatic Detection**: Captures all crash types (panic, watchdog, brownout)
- **Persistent Storage**: NVS-based circular buffer for crash logs
- **Startup Analysis**: Automatic crash log display at boot
- **Testing Framework**: Built-in crash triggers for debugging

**Debugging Workflow:**
1. **Reproduce Issue**: Use serial commands or normal operation
2. **Analyze Crash Logs**: Check startup output for stored crashes
3. **Review Stack Trace**: Examine backtrace in crash information
4. **Identify Root Cause**: Use crash context to locate bug
5. **Fix and Verify**: Implement fix and test thoroughly

### Performance Monitoring
**System Health Indicators:**
- **Heap Usage**: Monitor free heap and fragmentation
- **Task Performance**: Watch for watchdog timeouts
- **Memory Allocation**: Track SPIRAM vs DRAM usage
- **Network Stability**: Monitor WiFi connection reliability
- **UI Responsiveness**: Ensure smooth LVGL operations

## Git Workflow & Standards

### Commit Message Standards
**Format**: `<type>(<scope>): <emoji> <message>`

**Types & Emojis:**
- `feat`: ✨ New features and enhancements
- `fix`: 🐛 Bug fixes and error corrections
- `refactor`: ♻️ Code restructuring without functionality changes
- `docs`: 📝 Documentation updates
- `config`: ⚙️ Configuration and build system changes
- `perf`: 🚀 Performance improvements
- `ui`: 💄 User interface and styling changes
- `hardware`: 🔧 Hardware-related modifications

**Best Practices:**
- Keep messages concise and descriptive
- Use present tense ("add feature" not "added feature")
- Include scope when relevant (ui, wifi, serial, etc.)
- No commit body required - title should be self-explanatory
- Reference issues/PRs when applicable

### Development Branching
**Recommended Strategy:**
- **Main Branch**: Stable, working code only
- **Feature Branches**: Individual features and experiments
- **Hotfix Branches**: Critical bug fixes
- **Clean History**: Squash commits before merging when appropriate

## Hardware Integration Notes

### GPIO Configuration Strategy
**Pin Assignment Logic:**
- **RGB Interface**: Dedicated high-speed pins for display data
- **I2C Touch**: Standard I2C pins with proper pull-ups
- **System Pins**: Boot button, LED, and debug interfaces
- **Future Expansion**: Reserve pins for additional sensors/peripherals

### Power Management Considerations
**Optimization Targets:**
- **Display Backlight**: PWM control for brightness and power saving
- **Touch Sensitivity**: Proper interrupt handling for responsiveness
- **WiFi Power**: Efficient connection management
- **CPU Frequency**: Dynamic scaling based on workload

---
*Development documentation - Last updated: August 21, 2025*
