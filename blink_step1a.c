// Test: blink_minimal + GPIO
// Is GPIO init causing USB enumeration failure?
#include "pico/stdlib.h"
#include "hardware/gpio.h"  // UNCOMMENTED
#include "tusb.h"
#include "class/cdc/cdc_device.h"

#define GPIO_TRIGGER_OUT 4
#define GPIO_TRIGGER_IN  5

// USB VID/PID
#define USB_VID 0xCafe
#define USB_PID 0x4003

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
    // Config: number, interface count, string index, total length, attribute, power
    TUD_CONFIG_DESCRIPTOR(1, 3, 0, CONFIG_TOTAL_LEN, 0, 100),
    
    // CDC: itfnum, stridx, ep_notif, ep_notif_size, epout, epin, epsize
    TUD_CDC_DESCRIPTOR(0, 0, 0x81, 8, 0x02, 0x82, 64),
    
    // HID: itfnum, stridx, boot_protocol, report_desc_len, epin, epsize, ep_interval
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
    "Lenny Face GPIO Test",          // Different product name to test
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

// HID callbacks
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
    (void) instance; (void) report_id; (void) report_type; (void) buffer; (void) bufsize;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
    (void) instance; (void) report_id; (void) report_type; (void) buffer; (void) reqlen;
    return 0;
}

int main(void) {
    // Initialize USB FIRST
    tusb_init();
    
    // GPIO ENABLED
    gpio_init(GPIO_TRIGGER_OUT);
    gpio_set_dir(GPIO_TRIGGER_OUT, GPIO_OUT);
    gpio_put(GPIO_TRIGGER_OUT, 0);

    gpio_init(GPIO_TRIGGER_IN);
    gpio_set_dir(GPIO_TRIGGER_IN, GPIO_IN);
    gpio_pull_up(GPIO_TRIGGER_IN);

    bool short_detected = false;
    bool already_sent = false;
    uint32_t last_trigger_time = 0;
    
    while (true) {
        tud_task();

        bool is_shorted = (gpio_get(GPIO_TRIGGER_IN) == 0);

        if (is_shorted && !already_sent) {
            uint32_t current_time = to_ms_since_boot(get_absolute_time());

            if (is_shorted && !short_detected) {
                short_detected = true;
                last_trigger_time = current_time;
            }

            if (short_detected && (current_time - last_trigger_time > 50)) {
                // Would send lenny face here
                already_sent = true;
            }
        } else if (!is_shorted) {
            short_detected = false;
            already_sent = false;
        }
        // NO sleep_ms() - keep polling USB fast!
    }

    return 0;
}
