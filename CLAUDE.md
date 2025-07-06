# Claude AI Assistant - Project Documentation


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

**Configuration Workflow:**
1. Search for configuration option in `sdkconfig` to find the correct CONFIG name
2. Add or modify the setting in `sdkconfig.defaults.esp32s3`
3. Run `idf.py reconfigure` or clean build to apply changes
4. Verify the setting appears in the newly generated `sdkconfig`

**Example Configuration Categories:**
- Stack sizes: `CONFIG_ESP_MAIN_TASK_STACK_SIZE`, `CONFIG_FREERTOS_*_STACK_SIZE`
- Memory settings: `CONFIG_SPIRAM_*`, heap configurations
- Task priorities and watchdog timeouts
- Hardware peripherals: LCD, WiFi, UART settings
- Build optimizations and debug options

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
