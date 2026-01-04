// Lenny Face Keyboard - HID Only
// Types ( ͡° ͜ʖ ͡°) via GPIO trigger
// Supports Linux (Ctrl+Shift+U) and Windows (Alt+X)

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "tusb.h"

#define GPIO_TRIGGER_OUT      4    // Ground reference
#define GPIO_TRIGGER_LINUX    5    // Short to GPIO 4 for Linux mode
#define GPIO_TRIGGER_WINDOWS  6    // Short to GPIO 4 for Windows mode
#define GPIO_LED              25   // Pico onboard LED

// Debounce settings
#define DEBOUNCE_SAMPLES     8     // Number of consistent reads required
#define DEBOUNCE_INTERVAL_MS 10    // Time between samples
#define TRIGGER_COOLDOWN_MS  1000  // Minimum time between triggers

#define USB_VID 0xCafe
#define USB_PID 0x4003

// Device descriptor
tusb_desc_device_t const desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = 0x00,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = USB_VID,
    .idProduct          = USB_PID,
    .bcdDevice          = 0x0100,
    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x00,
    .bNumConfigurations = 0x01
};

uint8_t const *tud_descriptor_device_cb(void) {
    return (uint8_t const *)&desc_device;
}

// HID Report Descriptor
uint8_t const desc_hid_report[] = { TUD_HID_REPORT_DESC_KEYBOARD() };

uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance) {
    (void)instance;
    return desc_hid_report;
}

// Configuration descriptor
#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN)

uint8_t const desc_configuration[] = {
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, CONFIG_TOTAL_LEN, 0, 100),
    TUD_HID_DESCRIPTOR(0, 0, HID_ITF_PROTOCOL_KEYBOARD, sizeof(desc_hid_report), 0x81, 16, 10)
};

uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
    (void)index;
    return desc_configuration;
}

// String descriptors
char const *string_desc_arr[] = {
    (const char[]){0x09, 0x04},
    "Pico",
    "Lenny Face Keyboard",
};

static uint16_t _desc_str[32];

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void)langid;
    size_t chr_count;

    if (index == 0) {
        memcpy(&_desc_str[1], string_desc_arr[0], 2);
        chr_count = 1;
    } else {
        if (index >= sizeof(string_desc_arr) / sizeof(string_desc_arr[0]))
            return NULL;
        const char *str = string_desc_arr[index];
        chr_count = strlen(str);
        if (chr_count > 31) chr_count = 31;
        for (size_t i = 0; i < chr_count; i++) {
            _desc_str[1 + i] = str[i];
        }
    }

    _desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));
    return _desc_str;
}

// HID callbacks
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) {
    (void)instance; (void)report_id; (void)report_type; (void)buffer; (void)bufsize;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen) {
    (void)instance; (void)report_id; (void)report_type; (void)buffer; (void)reqlen;
    return 0;
}

//--------------------------------------------------------------------+
// Keyboard Functions
//--------------------------------------------------------------------+

void press_key(uint8_t modifier, uint8_t keycode) {
    uint8_t keys[6] = {keycode, 0, 0, 0, 0, 0};
    tud_hid_keyboard_report(0, modifier, keys);
    sleep_ms(20);
    tud_task();
}

void release_keys(void) {
    uint8_t keys[6] = {0};
    tud_hid_keyboard_report(0, 0, keys);
    sleep_ms(20);
    tud_task();
}

void type_key(uint8_t modifier, uint8_t keycode) {
    press_key(modifier, keycode);
    release_keys();
}

// Type ASCII character
void type_char(char c) {
    uint8_t keycode = 0, modifier = 0;

    if (c >= 'a' && c <= 'z') {
        keycode = HID_KEY_A + (c - 'a');
    } else if (c >= 'A' && c <= 'Z') {
        keycode = HID_KEY_A + (c - 'A');
        modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
    } else if (c >= '1' && c <= '9') {
        keycode = HID_KEY_1 + (c - '1');
    } else if (c == '0') {
        keycode = HID_KEY_0;
    } else {
        switch (c) {
            case ' ': keycode = HID_KEY_SPACE; break;
            case '(': keycode = HID_KEY_9; modifier = KEYBOARD_MODIFIER_LEFTSHIFT; break;
            case ')': keycode = HID_KEY_0; modifier = KEYBOARD_MODIFIER_LEFTSHIFT; break;
            case '_': keycode = HID_KEY_MINUS; modifier = KEYBOARD_MODIFIER_LEFTSHIFT; break;
            case '^': keycode = HID_KEY_6; modifier = KEYBOARD_MODIFIER_LEFTSHIFT; break;
            default: return;
        }
    }
    type_key(modifier, keycode);
}

// Type Unicode character using Linux Ctrl+Shift+U method
void type_unicode_linux(uint16_t codepoint) {
    // Press Ctrl+Shift+U
    uint8_t keys[6] = {HID_KEY_U, 0, 0, 0, 0, 0};
    tud_hid_keyboard_report(0, KEYBOARD_MODIFIER_LEFTCTRL | KEYBOARD_MODIFIER_LEFTSHIFT, keys);
    sleep_ms(30);
    tud_task();
    release_keys();
    sleep_ms(30);

    // Type hex digits
    char hex[5];
    snprintf(hex, sizeof(hex), "%04x", codepoint);
    for (int i = 0; i < 4; i++) {
        char c = hex[i];
        uint8_t keycode;
        if (c >= '0' && c <= '9') {
            keycode = (c == '0') ? HID_KEY_0 : HID_KEY_1 + (c - '1');
        } else {
            keycode = HID_KEY_A + (c - 'a');
        }
        type_key(0, keycode);
    }

    // Press Space to confirm
    type_key(0, HID_KEY_SPACE);
}

// Type Unicode character using Windows Alt+X method
// Works in Word, WordPad, and many other Windows apps
void type_unicode_windows(uint16_t codepoint) {
    // Type hex digits first
    char hex[5];
    snprintf(hex, sizeof(hex), "%04x", codepoint);
    for (int i = 0; i < 4; i++) {
        char c = hex[i];
        uint8_t keycode;
        if (c >= '0' && c <= '9') {
            keycode = (c == '0') ? HID_KEY_0 : HID_KEY_1 + (c - '1');
        } else {
            keycode = HID_KEY_A + (c - 'a');
        }
        type_key(0, keycode);
    }

    // Press Alt+X to convert
    uint8_t keys[6] = {HID_KEY_X, 0, 0, 0, 0, 0};
    tud_hid_keyboard_report(0, KEYBOARD_MODIFIER_LEFTALT, keys);
    sleep_ms(30);
    tud_task();
    release_keys();
    sleep_ms(30);
}

// Type the real Lenny face: ( ͡° ͜ʖ ͡°)
void type_lenny_face_linux(void) {
    if (!tud_hid_ready()) return;

    type_char('(');
    type_char(' ');
    type_unicode_linux(0x0361);  // ͡ combining double inverted breve
    type_unicode_linux(0x00b0);  // ° degree sign
    type_char(' ');
    type_unicode_linux(0x035c);  // ͜ combining double breve below
    type_unicode_linux(0x0296);  // ʖ latin letter inverted glottal stop
    type_char(' ');
    type_unicode_linux(0x0361);  // ͡
    type_unicode_linux(0x00b0);  // °
    type_char(' ');
    type_char(')');
}

void type_lenny_face_windows(void) {
    if (!tud_hid_ready()) return;

    type_char('(');
    type_char(' ');
    type_unicode_windows(0x0361);  // ͡ combining double inverted breve
    type_unicode_windows(0x00b0);  // ° degree sign
    type_char(' ');
    type_unicode_windows(0x035c);  // ͜ combining double breve below
    type_unicode_windows(0x0296);  // ʖ latin letter inverted glottal stop
    type_char(' ');
    type_unicode_windows(0x0361);  // ͡
    type_unicode_windows(0x00b0);  // °
    type_char(' ');
    type_char(')');
}

//--------------------------------------------------------------------+
// Debounce Logic
//--------------------------------------------------------------------+

typedef enum {
    STATE_IDLE,
    STATE_DEBOUNCING,
    STATE_TRIGGERED,
    STATE_COOLDOWN
} trigger_state_t;

static trigger_state_t state = STATE_IDLE;
static uint32_t state_start_time = 0;
static uint8_t debounce_count = 0;

// Read GPIO with multiple samples for reliability
typedef enum {
    TRIGGER_NONE,
    TRIGGER_LINUX,
    TRIGGER_WINDOWS
} trigger_mode_t;

trigger_mode_t read_trigger_stable(void) {
    int linux_count = 0;
    int windows_count = 0;
    
    for (int i = 0; i < 5; i++) {
        if (gpio_get(GPIO_TRIGGER_LINUX) == 0) linux_count++;
        if (gpio_get(GPIO_TRIGGER_WINDOWS) == 0) windows_count++;
        sleep_us(200);
    }
    
    // Require majority vote (at least 3 out of 5)
    if (linux_count >= 3) return TRIGGER_LINUX;
    if (windows_count >= 3) return TRIGGER_WINDOWS;
    return TRIGGER_NONE;
}

void led_blink(int times, int ms) {
    for (int i = 0; i < times; i++) {
        gpio_put(GPIO_LED, 1);
        sleep_ms(ms);
        gpio_put(GPIO_LED, 0);
        if (i < times - 1) sleep_ms(ms);
    }
}

//--------------------------------------------------------------------+
// Main
//--------------------------------------------------------------------+

int main(void) {
    tusb_init();

    // Setup trigger output (LOW) - ground reference
    gpio_init(GPIO_TRIGGER_OUT);
    gpio_set_dir(GPIO_TRIGGER_OUT, GPIO_OUT);
    gpio_put(GPIO_TRIGGER_OUT, 0);

    // Setup trigger inputs with pull-ups
    gpio_init(GPIO_TRIGGER_LINUX);
    gpio_set_dir(GPIO_TRIGGER_LINUX, GPIO_IN);
    gpio_pull_up(GPIO_TRIGGER_LINUX);
    
    gpio_init(GPIO_TRIGGER_WINDOWS);
    gpio_set_dir(GPIO_TRIGGER_WINDOWS, GPIO_IN);
    gpio_pull_up(GPIO_TRIGGER_WINDOWS);

    // Setup LED
    gpio_init(GPIO_LED);
    gpio_set_dir(GPIO_LED, GPIO_OUT);
    gpio_put(GPIO_LED, 0);

    // Wait for USB enumeration
    while (!tud_mounted()) {
        tud_task();
        sleep_ms(1);
    }

    // Signal ready with LED
    led_blink(3, 100);

    trigger_mode_t active_mode = TRIGGER_NONE;

    while (true) {
        tud_task();

        uint32_t now = to_ms_since_boot(get_absolute_time());
        trigger_mode_t current_trigger = read_trigger_stable();

        switch (state) {
            case STATE_IDLE:
                if (current_trigger != TRIGGER_NONE) {
                    state = STATE_DEBOUNCING;
                    state_start_time = now;
                    debounce_count = 1;
                    active_mode = current_trigger;
                }
                break;

            case STATE_DEBOUNCING:
                if (now - state_start_time >= DEBOUNCE_INTERVAL_MS) {
                    if (current_trigger == active_mode) {
                        debounce_count++;
                        if (debounce_count >= DEBOUNCE_SAMPLES) {
                            // Confirmed press - trigger!
                            gpio_put(GPIO_LED, 1);
                            
                            if (active_mode == TRIGGER_LINUX) {
                                // Blink once for Linux
                                led_blink(1, 100);
                                type_lenny_face_linux();
                            } else {
                                // Blink twice for Windows
                                led_blink(2, 50);
                                type_lenny_face_windows();
                            }
                            
                            gpio_put(GPIO_LED, 0);
                            state = STATE_TRIGGERED;
                            state_start_time = now;
                        } else {
                            state_start_time = now;
                        }
                    } else {
                        // Trigger changed or released, reset
                        state = STATE_IDLE;
                        debounce_count = 0;
                        active_mode = TRIGGER_NONE;
                    }
                }
                break;

            case STATE_TRIGGERED:
                // Wait for release
                if (current_trigger == TRIGGER_NONE) {
                    state = STATE_COOLDOWN;
                    state_start_time = now;
                }
                break;

            case STATE_COOLDOWN:
                // Prevent re-trigger for cooldown period
                if (now - state_start_time >= TRIGGER_COOLDOWN_MS) {
                    state = STATE_IDLE;
                    active_mode = TRIGGER_NONE;
                }
                break;
        }

        sleep_ms(2);  // Small delay to prevent CPU hogging
    }

    return 0;
}
