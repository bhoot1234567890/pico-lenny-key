# Raspberry Pi Pico Lenny Key

A single-key USB keyboard powered by Raspberry Pi Pico that types the legendary Lenny Face ( ͡° ͜ʖ ͡° ) when you short two GPIO pins.

Perfect for pranks, memes, or just having a dedicated Lenny button on your desk. 🎭

## Features

- **True USB HID Keyboard**: Appears as a standard keyboard device on Windows/Linux/macOS
- **Real Unicode Support**: Types the actual Lenny face ( ͡° ͜ʖ ͡° ) using Linux Unicode input method
- **Hardware Trigger**: Simple GPIO short circuit activation (GPIO 4 → GPIO 5)
- **Robust Debouncing**: Multi-sample stable read with cooldown to prevent accidental triggers
- **LED Feedback**: Onboard LED blinks to indicate ready state and typing
- **Debug Mode**: Built-in CDC serial output for troubleshooting

## Hardware

- **Required**: Raspberry Pi Pico or Pico W (RP2040)
- **Linux Mode**: Short GPIO 4 to GPIO 5
- **Windows Mode**: Short GPIO 4 to GPIO 6
- **Power**: USB connection provides power and data
- **LED**: Onboard LED (GPIO 25) for status indication

## How It Works

This project uses the RP2040's native USB controller with TinyUSB to implement a composite USB device:

### USB Device Architecture

**HID-Only Mode** (`lenny_keyboard.c`):
- Single USB HID keyboard interface
- Uses TinyUSB's `tud_hid_keyboard_report()` to send keypresses
- Minimal overhead, pure keyboard functionality

**Debug Mode** (`lenny_debug.c`):
- Composite USB device with CDC (serial) + HID (keyboard)
- CDC interface provides real-time debug output via serial port
- Allows monitoring GPIO state, debounce logic, and HID reports

### Unicode Input Method

The Lenny face contains non-ASCII characters. This project supports both Linux and Windows:

**Linux Mode** (GPIO 4 → GPIO 5):
Uses the **IBus/GTK Unicode input method**:
1. Press `Ctrl+Shift+U` to enter Unicode mode
2. Type the hexadecimal codepoint (e.g., `0361` for ◌͡ combining double inverted breve)
3. Press Space to confirm
4. Character is inserted

**Windows Mode** (GPIO 4 → GPIO 6):
Uses the **Alt+X method** (works in Word, WordPad, many apps):
1. Type the hexadecimal codepoint
2. Press `Alt+X` to convert to Unicode character

Both methods automatically type these codepoints:
- `0x0361` - ◌͡ combining double inverted breve
- `0x00b0` - ° degree sign
- `0x035c` - ◌͜ combining double breve below
- `0x0296` - ʖ latin letter inverted glottal stop

### Debounce Algorithm

Implements a 4-state finite state machine to ensure reliable triggering:

1. **IDLE**: Waiting for trigger signal
2. **DEBOUNCING**: Collecting multiple stable samples (default: 8 samples over 80ms)
3. **TRIGGERED**: Executing the Lenny face typing sequence
4. **COOLDOWN**: 1-second lockout period to prevent double-triggers

Each GPIO read uses majority voting across 5 samples to filter electrical noise.

## Usage

### Quick Start
1. Flash `lenny_keyboard.uf2` to your Pico
2. Plug into your computer
3. Wait for 3 LED blinks (device ready)
4. **For Linux**: Short GPIO 4 to GPIO 5 (LED blinks once)
5. **For Windows**: Short GPIO 4 to GPIO 6 (LED blinks twice)
6. Watch the Lenny face appear! ( ͡° ͜ʖ ͡° )

### Flashing Firmware
1. Hold BOOTSEL button while plugging in USB
2. Pico appears as USB mass storage device (RPI-RP2)
3. Drag and drop the `.uf2` file to the drive
4. Pico automatically reboots with new firmware

### Debug Mode
For troubleshooting or development:
```sh
# Flash debug version
# Then connect to serial port
screen /dev/tty.usbmodem* 115200

# Or use cat for simple output
cat /dev/tty.usbmodem*
```

Debug output shows:
- GPIO state (raw vs debounced)
- State machine transitions
- HID ready status
- Individual keystrokes being sent

## Building from Source

### Prerequisites
- [Pico SDK](https://github.com/raspberrypi/pico-sdk) installed
- ARM GCC toolchain
- CMake 3.13+

### Build Steps
```sh
# Set Pico SDK path
export PICO_SDK_PATH=/path/to/pico-sdk

# Build
mkdir build && cd build
cmake ..
make -j4

# Output files:
# - lenny_keyboard.uf2 (HID-only, production)
# - lenny_debug.uf2 (CDC+HID, debug)
```

### Configuration
Edit `lenny_keyboard.c` or `lenny_debug.c` to customize:
- `GPIO_TRIGGER_OUT` - Ground reference pin (default: GPIO 4)
- `GPIO_TRIGGER_LINUX` - Linux mode trigger (default: GPIO 5)
- `GPIO_TRIGGER_WINDOWS` - Windows mode trigger (default: GPIO 6)
- `DEBOUNCE_SAMPLES` - Number of consistent reads required
- `DEBOUNCE_INTERVAL_MS` - Time between debounce samples
- `TRIGGER_COOLDOWN_MS` - Minimum time between activations

## Technical Details

- **USB VID:PID**: `0xCafe:0x4003` (HID-only) / `0xCafe:0x4004` (Debug)
- **USB Device Class**: HID (keyboard) with optional CDC ACM (serial)
- **TinyUSB Configuration**: Custom `tusb_config.h` with optimized buffer sizes
- **Keyboard Report**: Standard boot protocol keyboard (6-key rollover)
- **Power Consumption**: <50mA typical

## Compatibility

✅ **Linux**: Full Unicode support via IBus/GTK (GPIO 4 → GPIO 5)  
✅ **Windows**: Alt+X method for Word/WordPad/compatible apps (GPIO 4 → GPIO 6)  
⚠️ **macOS**: Limited Unicode input support (alternative methods needed)

## Troubleshooting

**Device not recognized**: Check USB cable supports data (not just charging)  
**Lenny face appears as garbage**: Application may not support Unicode input method  
**Trigger doesn't work**: Verify GPIO pins are shorted properly, check debug output  
**Multiple triggers**: Increase `TRIGGER_COOLDOWN_MS` value

## License

MIT - Feel free to use, modify, and distribute!
