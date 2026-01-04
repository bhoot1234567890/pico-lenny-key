# lennykey

A Raspberry Pi Pico (RP2040) USB HID keyboard gadget that types the real Lenny Face ( ͡° ͜ʖ ͡° ) when triggered via GPIO. 

- **Plug and Play**: Appears as a keyboard on Windows/Linux.
- **Unicode Support**: Types the real Lenny face using Linux Unicode input (Ctrl+Shift+U + hex).
- **Hardware Debounce**: Reliable trigger via GPIO short (default: GPIO 4 to GPIO 5).
- **Debug Mode**: CDC serial output for troubleshooting.

## Hardware
- Raspberry Pi Pico or Pico W
- Short GPIO 4 to GPIO 5 to trigger
- Onboard LED blinks for feedback

## Usage
1. Build and flash `lenny_keyboard.uf2` to your Pico.
2. Plug into your PC. Wait for LED to blink 3× (ready).
3. Short GPIO 4 to GPIO 5. The Lenny face will be typed!

## Debugging
- Flash `lenny_debug.uf2` for serial debug output (CDC ACM).
- Connect with `screen /dev/tty.usbmodem* 115200` on macOS/Linux.

## Building
```sh
mkdir build && cd build
cmake ..
make -j4
```

## License
MIT
