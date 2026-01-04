# Lenny Face Keyboard - Android App

This Android app works with the Pico W firmware to enable full Unicode support on Android devices.

## How It Works

1. **Firmware sends UTF-8 data via USB CDC** - The Pico W sends `( ͡° ͜ʖ ͡°)` as UTF-8 bytes through the USB CDC (serial) interface
2. **Android app receives the data** - The app reads the CDC data from USB
3. **App copies to clipboard** - The received text is automatically copied to the Android clipboard
4. **User pastes** - The user can then paste the lenny face with Ctrl+V or long-press paste

## Building the App

### Prerequisites
- Android Studio
- Android SDK 21+ (Android 5.0 Lollipop and above)

### Steps
1. Open Android Studio
2. Select "Open an Existing Project"
3. Navigate to this `android` directory
4. Wait for Gradle sync to complete
5. Build and run on your Android device

**Note**: Your Android device must support USB OTG (On-The-Go) to connect USB peripherals.

## Installing the APK

If you have a pre-built APK:
1. Enable "Install from Unknown Sources" in your Android settings
2. Copy the APK to your device
3. Open the APK file to install

## Using the App

1. **Connect Pico W to Android** using USB OTG adapter or cable
2. **Open the app** - it should detect the Pico W automatically
3. **Grant USB permission** when prompted
4. **Short GP4 and GP5** on the Pico W
5. **The lenny face is copied to clipboard**
6. **Paste** in any text field using:
   - Long-press and select "Paste"
   - Ctrl+V (if using a physical keyboard)

## Architecture

The composite USB device has two interfaces:
- **HID Keyboard** (Interface 1): Sends Ctrl+V / Cmd+V paste commands
- **CDC Serial** (Interface 0): Sends UTF-8 string data

### Flow Diagram
```
[Pico W] --CDC--> [Android App] --Clipboard--> [System]
[Pico W] --HID--> [System] (Ctrl+V)
```

## Troubleshooting

**"USB device not found"**
- Make sure you're using a USB OTG adapter/cable
- Try unplugging and replugging the Pico W
- Check USB permission is granted

**"CDC interface not found"**
- Ensure the firmware is properly flashed
- Try a different USB cable
- Check if your device supports USB CDC

**Text doesn't paste**
- Make sure the app has copied text to clipboard (check status)
- Long-press in a text field and select "Paste"
- Some apps may have restrictions on clipboard paste

## Permissions

The app requires:
- USB Host API permission (requested on first use)
- USB device access (requested on connection)

## License

MIT License
