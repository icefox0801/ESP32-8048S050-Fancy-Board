# Claude AI Assistant - Project Documentation

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

---
*This documentation provides project overview and commit message guidelines for the ESP-IDF RGB LCD Panel project.*
