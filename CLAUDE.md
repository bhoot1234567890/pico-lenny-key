# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a Raspberry Pi Pico W USB device project using the Pico SDK and TinyUSB stack. When GPIO pins 4 and 5 are shorted together, the device outputs the lenny face `( ͡° ͜ʖ ͡°)`.

Two firmware versions are available:
- **lenny_keyboard** (`lenny_keyboard.c`): HID-only keyboard, production-ready
- **lenny_debug** (`lenny_debug.c`): CDC+HID composite with debug serial output

## Build System

This project uses CMake with the Pico SDK. The `PICO_SDK_PATH` environment variable must point to your Pico SDK installation.

### Build Both Versions

```bash
rm -rf build && mkdir build && cd build
cmake ..
make
```

Output: `lenny_keyboard.uf2` and `lenny_debug.uf2`

### Build Outputs

- `lenny_keyboard.uf2` - HID keyboard only (production firmware)
- `lenny_debug.uf2` - CDC serial + HID keyboard (debug firmware)

## Critical: USB Endpoint Configuration

**Endpoint Address Format:**
- Bit 7 = 1 for IN endpoints (device → host)
- Bit 7 = 0 for OUT endpoints (host → device)
- Bits 3-0 = endpoint number

### HID-Only Configuration (`lenny_keyboard.c`)

Uses endpoint 0x81 (IN endpoint 1), no conflicts with CDC.

```c
TUD_HID_DESCRIPTOR(0, 0, HID_ITF_PROTOCOL_KEYBOARD, sizeof(desc_hid_report), 0x81, 16, 10)
```

### CDC+HID Composite Configuration (`lenny_debug.c`)

```c
// CDC endpoints
#define EPNUM_CDC_NOTIF   0x81  // IN endpoint 1 (interrupt, notifications)
#define EPNUM_CDC_OUT     0x02  // OUT endpoint 2 (bulk, host → device)
#define EPNUM_CDC_IN      0x82  // IN endpoint 2 (bulk, device → host)
#define EPNUM_HID         0x83  // IN endpoint 3 (interrupt, HID)

// TUD_CDC_DESCRIPTOR(itfnum, stridx, ep_notif, ep_notif_size, epout, epin, epsize)
TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, 4, EPNUM_CDC_NOTIF, 8, EPNUM_CDC_OUT, EPNUM_CDC_IN, 64)

// TUD_HID_DESCRIPTOR(itfnum, stridx, protocol, desc_len, epin, size, interval)
TUD_HID_DESCRIPTOR(ITF_NUM_HID, 5, HID_ITF_PROTOCOL_KEYBOARD, sizeof(desc_hid_report), EPNUM_HID, 16, 10)
```

### Common Pitfalls
1. **Wrong direction bit**: `0x82` is IN, not OUT. CDC bulk OUT must be `0x02` (bit 7 = 0)
2. **Endpoint conflicts**: Each endpoint must have a unique address
3. **Interface numbering**: CDC uses 2 interfaces (comm + data), HID starts at interface 2
4. **HID endpoint size**: Use 16 bytes for HID (not 64) - matches standard boot keyboard

## TinyUSB Configuration

Two separate config files for different build targets:

| File | CDC Enabled | HID Enabled | Buffer Sizes | Used By |
|------|-------------|-------------|--------------|---------|
| `tusb_config_hid.h` | No | Yes | 16-byte HID | `lenny_keyboard` (production) |
| `tusb_config_debug.h` | Yes | Yes | 256-byte CDC RX/TX, 16-byte HID | `lenny_debug` (debug) |

The CMake build uses `target_compile_definitions` to set `TUSB_CONFIG_HEADER` per target, selecting the appropriate config file.

Both define:
- `CFG_TUD_HID 1` - HID keyboard enabled
- `CFG_TUSB_RHPORT0_MODE OPT_MODE_DEVICE` - Device mode only

## GPIO Configuration

- **GP4** (`GPIO_TRIGGER_OUT`) - Configured as output (set LOW)
- **GP5** (`GPIO_TRIGGER_IN`) - Configured as input with pull-up

When GP4 and GP5 are shorted together, GP5 reads LOW, triggering the lenny face output.

**Trigger detection**: Uses majority voting across 3-5 GPIO samples with 100-200us intervals to filter electrical noise.

**Debounce state machine**:
- `STATE_IDLE` - Waiting for trigger signal
- `STATE_DEBOUNCING` - Collecting stable samples (5-8 samples required)
- `STATE_TRIGGERED` - Executing typing sequence, waiting for release
- `STATE_COOLDOWN` - Lockout period (800-1000ms) to prevent double-triggers

**LED feedback**: GPIO 25 (onboard LED) blinks 3 times on ready, turns on during typing.

## Unicode Input Method

**Linux Ctrl+Shift+U method** (used by both firmware versions):

1. Press `Ctrl+Shift+U` to enter Unicode mode
2. Type 4-digit hex codepoint (e.g., `0361` for ͡)
3. Press Space to confirm
4. Character is inserted

**Codepoints used:**
- `0x0361` - ͡ combining double inverted breve
- `0x00b0` - ° degree sign
- `0x035c` - ͜ combining double breve below
- `0x0296` - ʖ latin letter inverted glottal stop

Note: The method works on Linux. Windows requires Alt+Numpad input and macOS requires "Unicode Hex Input" keyboard setting. The current implementation only supports the Ctrl+Shift+U method.

## USB Device Identification

| Firmware | VID | PID | Device Class | Product Name |
|----------|-----|-----|--------------|--------------|
| `lenny_keyboard` | 0xCafe | 0x4003 | HID (from interface) | "Lenny Face Keyboard" |
| `lenny_debug` | 0xCafe | 0x4004 | MISC/IAD (composite) | "Lenny Debug" |

## Using the Debug Version

The debug firmware includes CDC serial output for troubleshooting:

```bash
# Connect to serial port (macOS/Linux)
screen /dev/tty.usbmodem* 115200

# Or monitor output
cat /dev/tty.usbmodem*
```

Debug output shows:
- GPIO state (raw vs debounced)
- State machine transitions
- HID ready status
- Individual keystrokes being sent

## Usage Instructions

1. **Flash** `lenny_keyboard.uf2` to Pico W (hold BOOTSEL while plugging in USB, then copy .uf2 to RPI-RP2 drive)
2. **Connect** via USB to computer
3. **Wait** for 3 LED blinks (device ready)
4. **Open** a text editor
5. **Short GP4 and GP5** (or use a button between them)
6. **Lenny face appears!** (may take 2-3 seconds)

**Platform compatibility**:
- **Linux**: Full support via Ctrl+Shift+U Unicode input
- **Windows**: Requires modifying code to use Alt+Numpad method
- **macOS**: Requires "Unicode Hex Input" keyboard setting and code modifications

## Common Tasks

- **Clean build**: `rm -rf build && mkdir build && cd build && cmake .. && make`
- **Rebuild**: `make` in the build directory
- **Flash to Pico W**: Copy `.uf2` file to `/Volumes/RPI-RP2/` (macOS) or `RPI-RP2` (Windows/Linux) when Pico W is in BOOTSEL mode

## Architecture Notes

### Main Loop Structure

Both firmware versions follow this pattern:
1. Initialize USB (`tusb_init()`)
2. Initialize GPIO pins
3. Wait for USB enumeration (`tud_mounted()`)
4. Enter main loop:
   - Call `tud_task()` to process USB events
   - Check trigger state via debounce state machine
   - Type lenny face if triggered

### Key Functions

- `type_lenny_face()` - Main entry point, sequences the typing
- `type_char()` - Type single ASCII character via HID
- `type_unicode_linux()` - Type Unicode character via Ctrl+Shift+U
- `press_key()` / `release_keys()` - Low-level HID keyboard report functions
- `read_trigger_stable()` - GPIO read with noise filtering via majority vote
