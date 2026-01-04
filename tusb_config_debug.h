#ifndef TUSB_CONFIG_H
#define TUSB_CONFIG_H

#define CFG_TUSB_MCU          OPT_MCU_RP2040
#define CFG_TUSB_OS           OPT_OS_NONE
#define CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_ALIGN    __attribute__((aligned(4)))

// USB device mode on roothub port 0
#define CFG_TUSB_RHPORT0_MODE OPT_MODE_DEVICE

#define CFG_TUD_ENABLED       1
#define CFG_TUD_ENDPOINT0_SIZE 64

// Enable CDC + HID for debug
#define CFG_TUD_CDC           1
#define CFG_TUD_HID           1
#define CFG_TUD_MSC           0
#define CFG_TUD_MIDI          0
#define CFG_TUD_VENDOR        0

// CDC buffer sizes
#define CFG_TUD_CDC_RX_BUFSIZE 256
#define CFG_TUD_CDC_TX_BUFSIZE 256

// HID buffer
#define CFG_TUD_HID_EP_BUFSIZE 16

#endif
