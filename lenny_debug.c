// Lenny Face Keyboard - DEBUG VERSION with CDC serial output
// Types ( ͡° ͜ʖ ͡°) via GPIO trigger

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "tusb.h"

#define GPIO_TRIGGER_IN  5
#define GPIO_TRIGGER_OUT 4
#define GPIO_LED         25  // Pico onboard LED

// Debounce settings
#define DEBOUNCE_SAMPLES     8     // More samples for reliability
#define DEBOUNCE_INTERVAL_MS 10    // Longer interval
#define TRIGGER_COOLDOWN_MS  1000  // Longer cooldown

#define USB_VID 0xCafe
#define USB_PID 0x4004  // Different PID for debug version

//--------------------------------------------------------------------+
// USB Descriptors - CDC + HID composite
//--------------------------------------------------------------------+

tusb_desc_device_t const desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = TUSB_CLASS_MISC,
    .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol    = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = USB_VID,
    .idProduct          = USB_PID,
    .bcdDevice          = 0x0100,
    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,
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

// Configuration descriptor - CDC + HID
enum { ITF_NUM_CDC = 0, ITF_NUM_CDC_DATA, ITF_NUM_HID, ITF_NUM_TOTAL };
#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN + TUD_HID_DESC_LEN)
#define EPNUM_CDC_NOTIF   0x81
#define EPNUM_CDC_OUT     0x02
#define EPNUM_CDC_IN      0x82
#define EPNUM_HID         0x83

uint8_t const desc_configuration[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0, 100),
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, 4, EPNUM_CDC_NOTIF, 8, EPNUM_CDC_OUT, EPNUM_CDC_IN, 64),
    TUD_HID_DESCRIPTOR(ITF_NUM_HID, 5, HID_ITF_PROTOCOL_KEYBOARD, sizeof(desc_hid_report), EPNUM_HID, 16, 10)
};

uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
    (void)index;
    return desc_configuration;
}

// String descriptors
char const *string_desc_arr[] = {
    (const char[]){0x09, 0x04},
    "Pico",
    "Lenny Debug",
    "123456",
    "CDC",
    "HID",
};

static uint16_t _desc_str[32];

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void)langid;
    size_t chr_count;
    if (index == 0) {
        memcpy(&_desc_str[1], string_desc_arr[0], 2);
        chr_count = 1;
    } else {
        if (index >= sizeof(string_desc_arr) / sizeof(string_desc_arr[0])) return NULL;
        const char *str = string_desc_arr[index];
        chr_count = strlen(str);
        if (chr_count > 31) chr_count = 31;
        for (size_t i = 0; i < chr_count; i++) _desc_str[1 + i] = str[i];
    }
    _desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));
    return _desc_str;
}

// TinyUSB callbacks
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) {
    (void)instance; (void)report_id; (void)report_type; (void)buffer; (void)bufsize;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen) {
    (void)instance; (void)report_id; (void)report_type; (void)buffer; (void)reqlen;
    return 0;
}

//--------------------------------------------------------------------+
// Debug print via CDC
//--------------------------------------------------------------------+

void dbg_print(const char* str) {
    if (tud_cdc_connected()) {
        tud_cdc_write_str(str);
        tud_cdc_write_flush();
    }
}

void dbg_printf(const char* fmt, ...) {
    if (tud_cdc_connected()) {
        char buf[128];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        tud_cdc_write_str(buf);
        tud_cdc_write_flush();
    }
}

//--------------------------------------------------------------------+
// Keyboard Functions
//--------------------------------------------------------------------+

void press_key(uint8_t modifier, uint8_t keycode) {
    if (!tud_hid_ready()) {
        dbg_print("  [HID not ready!]\r\n");
        return;
    }
    uint8_t keys[6] = {keycode, 0, 0, 0, 0, 0};
    tud_hid_keyboard_report(0, modifier, keys);
    dbg_printf("  KEY: mod=0x%02X key=0x%02X\r\n", modifier, keycode);
    sleep_ms(25);
    tud_task();
}

void release_keys(void) {
    if (!tud_hid_ready()) return;
    uint8_t keys[6] = {0};
    tud_hid_keyboard_report(0, 0, keys);
    sleep_ms(25);
    tud_task();
}

void type_key(uint8_t modifier, uint8_t keycode) {
    press_key(modifier, keycode);
    release_keys();
}

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
    dbg_printf("CHAR '%c'\r\n", c);
    type_key(modifier, keycode);
}

void type_unicode_linux(uint16_t codepoint) {
    dbg_printf("UNICODE 0x%04X\r\n", codepoint);
    
    // Press Ctrl+Shift+U
    uint8_t keys[6] = {HID_KEY_U, 0, 0, 0, 0, 0};
    tud_hid_keyboard_report(0, KEYBOARD_MODIFIER_LEFTCTRL | KEYBOARD_MODIFIER_LEFTSHIFT, keys);
    sleep_ms(40);
    tud_task();
    release_keys();
    sleep_ms(40);

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

void type_lenny_face(void) {
    dbg_print("\r\n=== TYPING LENNY FACE ===\r\n");
    
    if (!tud_hid_ready()) {
        dbg_print("ERROR: HID not ready!\r\n");
        return;
    }

    type_char('(');
    type_char(' ');
    type_unicode_linux(0x0361);
    type_unicode_linux(0x00b0);
    type_char(' ');
    type_unicode_linux(0x035c);
    type_unicode_linux(0x0296);
    type_char(' ');
    type_unicode_linux(0x0361);
    type_unicode_linux(0x00b0);
    type_char(' ');
    type_char(')');
    
    dbg_print("=== DONE ===\r\n\r\n");
}

//--------------------------------------------------------------------+
// Debounce State Machine
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

const char* state_name(trigger_state_t s) {
    switch(s) {
        case STATE_IDLE: return "IDLE";
        case STATE_DEBOUNCING: return "DEBOUNCING";
        case STATE_TRIGGERED: return "TRIGGERED";
        case STATE_COOLDOWN: return "COOLDOWN";
        default: return "?";
    }
}

// Read raw GPIO value (no processing)
bool read_gpio_raw(void) {
    return gpio_get(GPIO_TRIGGER_IN) == 0;
}

// Read GPIO with multiple samples
bool read_trigger_stable(void) {
    int low_count = 0;
    for (int i = 0; i < 5; i++) {
        if (gpio_get(GPIO_TRIGGER_IN) == 0) low_count++;
        sleep_us(200);
    }
    return (low_count >= 3);
}

void led_on(void) { gpio_put(GPIO_LED, 1); }
void led_off(void) { gpio_put(GPIO_LED, 0); }

void led_blink(int times, int ms) {
    for (int i = 0; i < times; i++) {
        led_on(); sleep_ms(ms);
        led_off(); if (i < times - 1) sleep_ms(ms);
    }
}

//--------------------------------------------------------------------+
// Main
//--------------------------------------------------------------------+

int main(void) {
    tusb_init();

    // Setup GPIOs
    gpio_init(GPIO_TRIGGER_OUT);
    gpio_set_dir(GPIO_TRIGGER_OUT, GPIO_OUT);
    gpio_put(GPIO_TRIGGER_OUT, 0);

    gpio_init(GPIO_TRIGGER_IN);
    gpio_set_dir(GPIO_TRIGGER_IN, GPIO_IN);
    gpio_pull_up(GPIO_TRIGGER_IN);

    gpio_init(GPIO_LED);
    gpio_set_dir(GPIO_LED, GPIO_OUT);
    led_off();

    // Wait for USB enumeration
    while (!tud_mounted()) {
        tud_task();
        sleep_ms(1);
    }
    
    led_blink(3, 100);

    // Wait for CDC connection
    uint32_t cdc_wait_start = to_ms_since_boot(get_absolute_time());
    while (!tud_cdc_connected()) {
        tud_task();
        sleep_ms(10);
        // Timeout after 5 seconds
        if (to_ms_since_boot(get_absolute_time()) - cdc_wait_start > 5000) break;
    }

    dbg_print("\r\n\r\n");
    dbg_print("================================\r\n");
    dbg_print("  LENNY FACE KEYBOARD - DEBUG\r\n");
    dbg_print("================================\r\n");
    dbg_printf("GPIO IN:  %d (pull-up)\r\n", GPIO_TRIGGER_IN);
    dbg_printf("GPIO OUT: %d (always LOW)\r\n", GPIO_TRIGGER_OUT);
    dbg_print("Short GPIO 4 to GPIO 5 to trigger\r\n");
    dbg_print("--------------------------------\r\n\r\n");

    trigger_state_t prev_state = state;
    uint32_t last_status = 0;
    uint32_t last_gpio_print = 0;

    while (true) {
        tud_task();

        uint32_t now = to_ms_since_boot(get_absolute_time());
        bool raw = read_gpio_raw();
        bool stable = read_trigger_stable();

        // Print GPIO status every 500ms when something is happening
        if (raw && (now - last_gpio_print > 500)) {
            dbg_printf("[%lu] GPIO: raw=%d stable=%d state=%s count=%d\r\n", 
                       now, raw, stable, state_name(state), debounce_count);
            last_gpio_print = now;
        }

        // State machine
        switch (state) {
            case STATE_IDLE:
                if (stable) {
                    state = STATE_DEBOUNCING;
                    state_start_time = now;
                    debounce_count = 1;
                    dbg_printf("[%lu] -> DEBOUNCING (count=1)\r\n", now);
                }
                break;

            case STATE_DEBOUNCING:
                if (now - state_start_time >= DEBOUNCE_INTERVAL_MS) {
                    if (stable) {
                        debounce_count++;
                        dbg_printf("[%lu] DEBOUNCE count=%d/%d\r\n", now, debounce_count, DEBOUNCE_SAMPLES);
                        
                        if (debounce_count >= DEBOUNCE_SAMPLES) {
                            dbg_printf("[%lu] -> TRIGGERED!\r\n", now);
                            led_on();
                            type_lenny_face();
                            led_off();
                            state = STATE_TRIGGERED;
                            state_start_time = now;
                        } else {
                            state_start_time = now;
                        }
                    } else {
                        dbg_printf("[%lu] NOISE RESET (was at count=%d)\r\n", now, debounce_count);
                        state = STATE_IDLE;
                        debounce_count = 0;
                    }
                }
                break;

            case STATE_TRIGGERED:
                if (!stable) {
                    dbg_printf("[%lu] -> COOLDOWN (released)\r\n", now);
                    state = STATE_COOLDOWN;
                    state_start_time = now;
                }
                break;

            case STATE_COOLDOWN:
                if (now - state_start_time >= TRIGGER_COOLDOWN_MS) {
                    dbg_printf("[%lu] -> IDLE (cooldown done)\r\n", now);
                    state = STATE_IDLE;
                }
                break;
        }

        // Print status every 10 seconds
        if (now - last_status > 10000) {
            dbg_printf("[%lu] STATUS: state=%s gpio_raw=%d gpio_stable=%d\r\n", 
                       now, state_name(state), raw, stable);
            last_status = now;
        }

        sleep_ms(2);
    }

    return 0;
}
