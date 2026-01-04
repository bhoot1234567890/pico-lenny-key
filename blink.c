// Lenny Face Keyboard - CDC + HID Composite Device
// Based on working blink_gpio_test.c
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "tusb.h"
#include "class/cdc/cdc_device.h"

#define GPIO_TRIGGER_OUT 4
#define GPIO_TRIGGER_IN  5
#define GPIO_DEBUG_LED   25  // On-board LED (regular Pico) or external LED

// USB VID/PID
#define USB_VID 0xCafe
#define USB_PID 0x4003

// Lenny face UTF-8 string
#define LENNY_FACE_UTF8 "( ͡° ͜ʖ ͡°)"

// Device descriptor with IAD for composite device
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
    .iSerialNumber      = 0x00,
    .bNumConfigurations = 0x01
};

uint8_t const * tud_descriptor_device_cb(void) {
    return (uint8_t const *) &desc_device;
}

// HID Report Descriptor
uint8_t const desc_hid_report[] = { TUD_HID_REPORT_DESC_KEYBOARD() };

uint8_t const * tud_hid_descriptor_report_cb(uint8_t instance) {
    (void) instance;
    return desc_hid_report;
}

// Configuration descriptor - CDC + HID
#define CONFIG_TOTAL_LEN  (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN + TUD_HID_DESC_LEN)

uint8_t const desc_configuration[] = {
    TUD_CONFIG_DESCRIPTOR(1, 3, 0, CONFIG_TOTAL_LEN, 0, 100),
    TUD_CDC_DESCRIPTOR(0, 0, 0x81, 8, 0x02, 0x82, 64),
    TUD_HID_DESCRIPTOR(2, 0, HID_ITF_PROTOCOL_KEYBOARD, sizeof(desc_hid_report), 0x83, 16, 10)
};

uint8_t const * tud_descriptor_configuration_cb(uint8_t index) {
    (void) index;
    return desc_configuration;
}

// String descriptors
char const *string_desc_arr[] = {
    (const char[]) { 0x09, 0x04 },  // Language ID
    "Pico W",                        // Manufacturer
    "Lenny Face Keyboard",           // Product
};

static uint16_t _desc_str[32];

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void) langid;
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

    _desc_str[0] = (uint16_t) ((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));
    return _desc_str;
}

// HID callbacks (required)
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
    (void) instance; (void) report_id; (void) report_type; (void) buffer; (void) bufsize;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
    (void) instance; (void) report_id; (void) report_type; (void) buffer; (void) reqlen;
    return 0;
}

//--------------------------------------------------------------------+
// CDC Functions
//--------------------------------------------------------------------+

void cdc_send_string(const char *str) {
    if (!tud_cdc_connected()) return;

    uint32_t start_time = to_ms_since_boot(get_absolute_time());
    while (!tud_cdc_ready() && (to_ms_since_boot(get_absolute_time()) - start_time < 1000)) {
        tud_task();
        sleep_ms(10);
    }

    if (tud_cdc_ready()) {
        tud_cdc_write_str(str);
        tud_cdc_write_flush();
    }
}

//--------------------------------------------------------------------+
// HID Keyboard Functions  
//--------------------------------------------------------------------+

void send_key(uint8_t modifier, uint8_t keycode) {
    if (!tud_hid_ready()) return;

    uint8_t keycode_array[6] = {0};
    keycode_array[0] = keycode;

    tud_hid_keyboard_report(0, modifier, keycode_array);
    sleep_ms(50);

    keycode_array[0] = 0;
    tud_hid_keyboard_report(0, 0, keycode_array);
    sleep_ms(50);
}

void send_paste(void) {
    if (!tud_hid_ready()) return;

    uint8_t keycode_array[6] = {0};
    
    // Ctrl+V for Windows/Linux
    keycode_array[0] = HID_KEY_V;
    tud_hid_keyboard_report(0, KEYBOARD_MODIFIER_LEFTCTRL, keycode_array);
    sleep_ms(50);
    keycode_array[0] = 0;
    tud_hid_keyboard_report(0, 0, keycode_array);
    sleep_ms(100);

    // Cmd+V for macOS
    keycode_array[0] = HID_KEY_V;
    tud_hid_keyboard_report(0, KEYBOARD_MODIFIER_LEFTGUI, keycode_array);
    sleep_ms(50);
    keycode_array[0] = 0;
    tud_hid_keyboard_report(0, 0, keycode_array);
    sleep_ms(100);
}

// Helper to type a single character
void type_char(char c) {
    if (!tud_hid_ready()) return;
    
    uint8_t keycode = 0;
    uint8_t modifier = 0;
    
    // Map ASCII to HID keycodes
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
            case '^': keycode = HID_KEY_6; modifier = KEYBOARD_MODIFIER_LEFTSHIFT; break;
            case '_': keycode = HID_KEY_MINUS; modifier = KEYBOARD_MODIFIER_LEFTSHIFT; break;
            case '-': keycode = HID_KEY_MINUS; break;
            case '.': keycode = HID_KEY_PERIOD; break;
            case ',': keycode = HID_KEY_COMMA; break;
            case '/': keycode = HID_KEY_SLASH; break;
            case '\\': keycode = HID_KEY_BACKSLASH; break;
            case '\'': keycode = HID_KEY_APOSTROPHE; break;
            case '"': keycode = HID_KEY_APOSTROPHE; modifier = KEYBOARD_MODIFIER_LEFTSHIFT; break;
            case ';': keycode = HID_KEY_SEMICOLON; break;
            case ':': keycode = HID_KEY_SEMICOLON; modifier = KEYBOARD_MODIFIER_LEFTSHIFT; break;
            case '!': keycode = HID_KEY_1; modifier = KEYBOARD_MODIFIER_LEFTSHIFT; break;
            case '@': keycode = HID_KEY_2; modifier = KEYBOARD_MODIFIER_LEFTSHIFT; break;
            case '#': keycode = HID_KEY_3; modifier = KEYBOARD_MODIFIER_LEFTSHIFT; break;
            case '$': keycode = HID_KEY_4; modifier = KEYBOARD_MODIFIER_LEFTSHIFT; break;
            case '%': keycode = HID_KEY_5; modifier = KEYBOARD_MODIFIER_LEFTSHIFT; break;
            case '&': keycode = HID_KEY_7; modifier = KEYBOARD_MODIFIER_LEFTSHIFT; break;
            case '*': keycode = HID_KEY_8; modifier = KEYBOARD_MODIFIER_LEFTSHIFT; break;
            case '=': keycode = HID_KEY_EQUAL; break;
            case '+': keycode = HID_KEY_EQUAL; modifier = KEYBOARD_MODIFIER_LEFTSHIFT; break;
            case '[': keycode = HID_KEY_BRACKET_LEFT; break;
            case ']': keycode = HID_KEY_BRACKET_RIGHT; break;
            case '{': keycode = HID_KEY_BRACKET_LEFT; modifier = KEYBOARD_MODIFIER_LEFTSHIFT; break;
            case '}': keycode = HID_KEY_BRACKET_RIGHT; modifier = KEYBOARD_MODIFIER_LEFTSHIFT; break;
            case '`': keycode = HID_KEY_GRAVE; break;
            case '~': keycode = HID_KEY_GRAVE; modifier = KEYBOARD_MODIFIER_LEFTSHIFT; break;
            case '<': keycode = HID_KEY_COMMA; modifier = KEYBOARD_MODIFIER_LEFTSHIFT; break;
            case '>': keycode = HID_KEY_PERIOD; modifier = KEYBOARD_MODIFIER_LEFTSHIFT; break;
            case '?': keycode = HID_KEY_SLASH; modifier = KEYBOARD_MODIFIER_LEFTSHIFT; break;
            case '|': keycode = HID_KEY_BACKSLASH; modifier = KEYBOARD_MODIFIER_LEFTSHIFT; break;
            case '\n': keycode = HID_KEY_ENTER; break;
            case '\t': keycode = HID_KEY_TAB; break;
            default: return; // Skip unknown characters
        }
    }
    
    uint8_t keycode_array[6] = {0};
    keycode_array[0] = keycode;
    
    tud_hid_keyboard_report(0, modifier, keycode_array);
    sleep_ms(30);
    tud_task();
    
    keycode_array[0] = 0;
    tud_hid_keyboard_report(0, 0, keycode_array);
    sleep_ms(30);
    tud_task();
}

// Type a string character by character
void type_string(const char* str) {
    while (*str) {
        type_char(*str++);
    }
}

void send_lenny_face(void) {
    if (!tud_mounted()) return;
    
    if (tud_cdc_ready()) {
        tud_cdc_write_str("Sending Lenny face!\r\n");
        tud_cdc_write_flush();
    }
    
    // Type an ASCII-art Lenny face approximation
    // ( . Y . ) or similar - let's use a recognizable one
    type_string("( ^_^ )");
    
    if (tud_cdc_ready()) {
        tud_cdc_write_str("Done!\r\n");
        tud_cdc_write_flush();
    }
}


//--------------------------------------------------------------------+
// Main
//--------------------------------------------------------------------+

int main(void) {
    tusb_init();
    
    gpio_init(GPIO_TRIGGER_OUT);
    gpio_set_dir(GPIO_TRIGGER_OUT, GPIO_OUT);
    gpio_put(GPIO_TRIGGER_OUT, 0);

    gpio_init(GPIO_TRIGGER_IN);
    gpio_set_dir(GPIO_TRIGGER_IN, GPIO_IN);
    gpio_pull_up(GPIO_TRIGGER_IN);

    // Debug LED to show trigger detection
    gpio_init(GPIO_DEBUG_LED);
    gpio_set_dir(GPIO_DEBUG_LED, GPIO_OUT);
    gpio_put(GPIO_DEBUG_LED, 0);

    bool already_sent = false;
    uint32_t last_trigger_time = 0;
    const uint32_t DEBOUNCE_MS = 500;  // 500ms cooldown between triggers

    while (true) {
        tud_task();

        bool is_shorted = (gpio_get(GPIO_TRIGGER_IN) == 0);
        uint32_t current_time = to_ms_since_boot(get_absolute_time());
        
        // Debug: Toggle LED when shorted
        gpio_put(GPIO_DEBUG_LED, is_shorted ? 1 : 0);

        // Only process triggers if USB is mounted and debounce time has passed
        if (tud_mounted() && is_shorted && !already_sent) {
            send_lenny_face();
            already_sent = true;
            last_trigger_time = current_time;
        }
        
        // Reset after releasing AND waiting for debounce period
        if (!is_shorted && already_sent && (current_time - last_trigger_time > DEBOUNCE_MS)) {
            already_sent = false;
        }

        // Only sleep after USB is fully mounted - sleeping during enumeration causes failures!
        if (tud_mounted()) {
            sleep_ms(10);
        }
    }

    return 0;
}
