# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a Raspberry Pi Pico W USB device project using the Pico SDK and TinyUSB stack. When GPIO pins 4 and 5 are shorted together, the device outputs the lenny face `( ͡° ͜ʖ ͡°)`.

Two firmware versions are available:
- **HID-only**: Plug-and-play on Windows/Linux (no app required)
- **CDC+HID composite**: For Android/macOS with helper app

## Build System

This project uses CMake with the Pico SDK. The `PICO_SDK_PATH` environment variable must point to your Pico SDK installation.

### Build Both Versions

```bash
cd build
cmake .. -DBOTH_MODES=ON
make
```

Output: `blink_cdc_hid.uf2` and `blink_hid_only.uf2`

### Build Single Version (CDC+HID)

```bash
cd build
cmake ..
make
```

Output: `blink.uf2` (CDC+HID composite), plus test executables

### Build Outputs

The build process generates UF2 files:
- `blink_cdc_hid.uf2` - CDC serial + HID keyboard (for Android/macOS)
- `blink_hid_only.uf2` - HID keyboard only (Windows/Linux plug-and-play)
- Test executables (when not using BOTH_MODES): `test_usb.uf2`, `test_cdc.uf2`, `test_cdc_hid.uf2`, `blink_minimal.uf2`, `blink_gpio_test.uf2`

## Critical: USB Endpoint Configuration

**This has been the major source of bugs.** When modifying USB descriptors, follow these rules:

### Endpoint Address Format
- Bit 7 = 1 for IN endpoints (device → host)
- Bit 7 = 0 for OUT endpoints (host → device)
- Bits 3-0 = endpoint number

### CDC+HID Composite Working Configuration (blink.c:65)

```c
// TUD_CDC_DESCRIPTOR(itfnum, stridx, ep_notif, ep_notif_size, epout, epin, epsize)
TUD_CDC_DESCRIPTOR(0, 0, 0x81, 8, 0x02, 0x82, 64)

// Breakdown:
// - ep_notif: 0x81 = IN endpoint 1 (interrupt for CDC notifications)
// - ep_notif_size: 8 bytes
// - epout: 0x02 = OUT endpoint 2 (host → device, bulk)
// - epin: 0x82 = IN endpoint 2 (device → host, bulk)
// - epsize: 64 bytes for bulk endpoints
```

```c
// TUD_HID_DESCRIPTOR(itfnum, stridx, protocol, desc_len, epin, size, interval)
TUD_HID_DESCRIPTOR(2, 0, HID_ITF_PROTOCOL_KEYBOARD, sizeof(desc_hid_report), 0x83, 16, 10)

// Breakdown:
// - itfnum: 2 (CDC uses interfaces 0 and 1)
// - epin: 0x83 = IN endpoint 3 (device → host, interrupt)
// - size: 16 bytes (NOT 64 - matches working test_cdc_hid.c)
```

### Common Pitfalls
1. **Wrong direction bit**: `0x82` is IN, not OUT. CDC bulk OUT must be `0x02` (bit 7 = 0)
2. **Endpoint conflicts**: Each endpoint must have a unique address
3. **Wrong macro signature**: `TUD_CDC_DESCRIPTOR` has 7 parameters, not 6
4. **Interface numbering**: CDC uses 2 interfaces (comm + data), HID starts at interface 2

### HID-Only Configuration (blink_hid.c:56)

```c
TUD_HID_DESCRIPTOR(0, 0, HID_ITF_PROTOCOL_KEYBOARD, sizeof(desc_hid_report), 0x81, CFG_TUD_HID_EP_BUFSIZE, 10)
```

Uses endpoint 0x81, no conflicts with CDC.

## TinyUSB Configuration Files

Four separate config files for different build targets:

| File | CDC Enabled | Buffer Sizes | Used By |
|------|-------------|--------------|---------|
| `tusb_config.h` | Yes | 64 bytes all | Default single build (`blink.uf2`) |
| `tusb_config_cdc.h` | Yes | 256-byte RX/TX | Dual build CDC+HID |
| `tusb_config_hid.h` | No | N/A | Dual build HID-only |
| `tusb_config_test.h` | No | 16-byte HID | `test_usb.uf2` (minimal USB test) |

All define: `CFG_TUD_HID 1` and `CFG_TUSB_RHPORT0_MODE (OPT_MODE_DEVICE | ...)`

## GPIO Configuration

- **GP4** - Configured as output (set LOW)
- **GP5** - Configured as input with pull-up

When GP4 and GP5 are shorted together, GP5 reads LOW, triggering the lenny face output. The code includes 50ms debouncing and one-shot behavior.

**Critical initialization order**: In `blink.c`, `tusb_init()` is called BEFORE GPIO initialization. This matters for USB enumeration timing.

**Note**: In `blink_hid.c`, `tusb_init()` is called AFTER GPIO initialization. This still works for HID-only because HID enumeration is more lenient than CDC.

**Also note**: `blink_hid.c` calls `stdio_init_all()` which initializes UART. This doesn't interfere with USB HID operation.

**CRITICAL: Main Loop Sleep Timing**

USB enumeration fails if `sleep_ms()` blocks during the enumeration phase. The main loop must call `tud_task()` frequently during enumeration.

```c
// WRONG - blocks during enumeration causing USB timeout:
sleep_ms(10);

// CORRECT - only sleep AFTER USB is mounted:
if (tud_mounted()) {
    sleep_ms(10);
}
```

The USB host expects rapid responses during enumeration. Blocking for even 10ms per iteration causes enumeration failures and the device won't be recognized.

## Which Version Should I Use?

| Platform | Use This Version | Notes |
|----------|-------------------|-------|
| **Windows** | `blink_hid_only.uf2` | Plug-and-play, no app needed |
| **Linux** | `blink_hid_only.uf2` | Plug-and-play, no app needed |
| **macOS** | `blink_hid_only.uf2` | Requires "Unicode Hex Input" keyboard setting |
| **Android** | `blink_cdc_hid.uf2` | Requires Android app (see `android/` directory) |
| **iOS** | Not supported | - |

## How Each Version Works

### HID-Only Version (`blink_hid.c`)

Sends the lenny face using platform-specific Unicode input methods:

**Windows (Alt+NumPad):**
```
Alt (hold) → NumPad + → 0 → 3 → 6 → 1 → Alt (release) = ͡
```

**Linux (Ctrl+Shift+U):**
```
Ctrl+Shift+U → 0 → 3 → 6 → 1 → Enter = ͡
```

The device tries both methods in sequence - each platform responds to the method it supports and ignores the other.

### CDC+HID Composite Version (`blink.c`)

**Current implementation**: The main `blink.c` sends a simplified ASCII face "( ^_^ )" using HID keyboard typing.

**Intended design** (documented in comments): Should send `( ͡° ͜ʖ ͡°)` via CDC serial, then send Ctrl+V / Cmd+V via HID keyboard. For this to work, a helper app would need to:
- Receive CDC data
- Copy it to clipboard
- Then the Ctrl+V paste works

Note: The CDC+HID composite is intended for platforms that need a helper app (Android/macOS).

## USB Device Identification

| Firmware | VID | PID | Device Class |
|----------|-----|-----|--------------|
| CDC+HID | 0xCafe | 0x4003 | MISC/IAD (composite) |
| HID-only | 0xCafe | 0x4001 | 0x00 (from interface) |

Product name: "Lenny Face Keyboard"

## Test Executables (Single Build Mode)

When building without `BOTH_MODES`, several test executables are created:
- `test_usb.uf2` - Minimal USB enumeration test
- `test_cdc.uf2` - CDC-only test
- `test_cdc_hid.uf2` - Minimal working CDC+HID (reference for endpoint config)
- `blink_minimal.uf2` - Same as test_cdc_hid
- `blink_gpio_test.uf2` - Tests if GPIO initialization affects USB

These are useful for isolating USB enumeration issues.

## Usage Instructions

### For Windows/Linux Desktop

1. **Flash** `blink_hid_only.uf2` to Pico W
2. **Connect** via USB
3. **Open** a text editor
4. **Short GP4 and GP5**
5. **Lenny face appears!** `( ͡° ͜ʖ ͡°)`

### For Android

1. **Build/install the Android app** from `android/` directory
2. **Flash** `blink_cdc_hid.uf2` to Pico W
3. **Connect** Pico W to Android via USB OTG
4. **Open** the Android app
5. **Short GP4 and GP5**
6. **Lenny face is copied to clipboard**
7. **Paste** with Ctrl+V or long-press

## Android App

Located in `android/` directory:
- `MainActivity.java` - USB Host API implementation, receives CDC data and copies to clipboard
- `AndroidManifest.xml` - USB host permissions and device attachment filter
- `device_filter.xml` - Filters for VID 0xCafe, PID 0x4003

## Common Tasks

- **Clean build both versions**: `rm -rf build && mkdir build && cd build && cmake .. -DBOTH_MODES=ON && make`
- **Rebuild**: `make` in the build directory
- **Flash to Pico W**: Copy `.uf2` file to `/Volumes/RPI-RP2/` (macOS) when Pico W is in BOOTSEL mode

## Technical Details

### Unicode Code Points Used

- `(` = Shift+9 (standard key)
- ` ` = Space (standard key)
- `͡` = U+0361 (Combining Double Inverted Breve)
- `°` = U+00B0 (Degree Sign)
- `͜` = U+035C (Combining Double Breve Below)
- `ʖ` = U+02A6 (Latin Small Letter L with Middle Tilde)
- `)` = Shift+0 (standard key)

### HID-Only Timing

Each character takes approximately:
- Standard keys: 100ms (press + release)
- Unicode characters: ~300ms per character
- Total lenny face time: ~3.5 seconds

### CDC+HID Timing

- CDC transmission: ~50ms
- Wait for app processing: 500ms
- Paste command: 150ms
- Total: ~700ms
