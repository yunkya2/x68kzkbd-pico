/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021, Ha Thach (tinyusb.org)
 * Copyright 2023 Yuichi Nakamura (for x68kzkbd-pico)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "bsp/board.h"
#include "tusb.h"

//--------------------------------------------------------------------+
// MACRO TYPEDEF CONSTANT ENUM DECLARATION
//--------------------------------------------------------------------+

/* X68000Z keyboard Vendor & Product ID */
#define X68000ZKBD_VID    0x33dd
#define X68000ZKBD_PID    0x0011

#define MAX_REPORT  8

// Each HID instance can has multiple reports
static struct
{
  uint8_t report_count;
  tuh_hid_report_info_t report_info[MAX_REPORT];
}hid_info[CFG_TUH_HID];

static void process_kbd_report(hid_keyboard_report_t const *report);
static void process_generic_report(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len);

static int keybd_dev_addr = 0xff;

//--------------------------------------------------------------------+
// Keyboard LED control
//--------------------------------------------------------------------+

static int report_ready = 0;
static int report_x68000z = 0;

static int led_report_done = 1;
static int led_report_update = 1;
static uint8_t led_report_data[65];

void tuh_hid_set_report_complete_cb(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type, uint16_t len)
{
//  printf("report done\n");
  led_report_done = 1;
}

static void led_report_task(void)
{
  if (!report_ready)
    return;

  if (!led_report_update || !led_report_done)
    return;

//  printf("report\n");
  if (report_x68000z) {
    tuh_hid_set_report(keybd_dev_addr, 1, 10,
                       HID_REPORT_TYPE_FEATURE, led_report_data, sizeof(led_report_data));
  }
  
  led_report_update = led_report_done = 0;
}

static void led_report_set(uint64_t led)
{
  int i;

  printf("LED: %014llx\n", led);

  led_report_data[0] = 10;
  led_report_data[1] = 0xf8;
  for (i = 0; i < 8; i++) {
    if (i == 5)
      continue;
    led_report_data[7 + i] = led & 0xff;
    led >>= 8;
  }

  led_report_update = 1;
}

//--------------------------------------------------------------------+
// TinyUSB Callbacks
//--------------------------------------------------------------------+

void tuh_mount_cb(uint8_t dev_addr)
{
  // application set-up
  printf("A device with address %d is mounted\r\n", dev_addr);

  uint16_t vid, pid;
  tuh_vid_pid_get(dev_addr, &vid, &pid);
  printf("Vendor ID: %04x  Product ID: %04x\r\n", vid, pid);
  if (vid == X68000ZKBD_VID && pid == X68000ZKBD_PID) {
    printf("X68000Z keyboard connected\n");
    report_x68000z = 1;
  } else {
    printf("Generic keyboard connected\n");
    report_x68000z = 0;
  }

  report_ready = 1;
}

void tuh_umount_cb(uint8_t dev_addr)
{
  // application tear-down
  printf("A device with address %d is unmounted \r\n", dev_addr);
  report_ready = 0;
}

//--------------------------------------------------------------------+
// App main task
//--------------------------------------------------------------------+

static uint64_t demo_pattern[] = {
  0x00000000000000, 0x000000000000ff, 0x0000000000ffff, 0x00000000ffffff, 0x000000ffffffff,
  0x0000ffffffffff, 0x00ffffffffffff, 0xffffffffffffff, 0xffffffffffff00, 0xffffffffff0000,
  0xffffffff000000, 0xffffff00000000, 0xffff0000000000, 0xff000000000000, 0x00000000000000,
  0x00000000000000, 0x10101010101010, 0x20202020202020, 0x30303030303030, 0x40404040404040,
  0x50505050505050, 0x80808080808080, 0xf0f0f0f0f0f0f0, 0xffffffffffffff, 0xf0f0f0f0f0f0f0,
  0x80808080808080, 0x50505050505050, 0x40404040404040, 0x30303030303030, 0x20202020202020,
  0x10101010101010, 0x00000000000000,
  0x00000000000010, 0x00000000001020, 0x00000000102030, 0x00000010203040, 0x00001020304050,
  0x00102030405080, 0x102030405080f0, 0x2030405080f0ff, 0x30405080f0fff0, 0x405080f0fff080,
  0x5080f0fff08050, 0x80f0fff0805040, 0xf0fff080504030, 0xfff08050403020, 0xf0805040302010,
  0x80504030201000, 0x50403020100000, 0x40302010000000, 0x30201000000000, 0x20100000000000,
  0x10000000000000, 0x00000000000000,
};

void hid_app_task(void)
{
  const uint32_t interval_ms = 200;
  static uint32_t start_ms = 0;

  if (!report_ready)
    return;

  led_report_task();
  if ( board_millis() - start_ms < interval_ms) return; // not enough time
  start_ms += interval_ms;

  static int i = 0;

  led_report_set(demo_pattern[i]);
  i++;
  if (i >= (sizeof(demo_pattern) / sizeof(demo_pattern[0]))) {
    i = 0;
  }
}

//--------------------------------------------------------------------+
// TinyUSB Callbacks
//--------------------------------------------------------------------+

// Invoked when device with hid interface is mounted
// Report descriptor is also available for use. tuh_hid_parse_report_descriptor()
// can be used to parse common/simple enough descriptor.
// Note: if report descriptor length > CFG_TUH_ENUMERATION_BUFSIZE, it will be skipped
// therefore report_desc = NULL, desc_len = 0
void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len)
{
  printf("HID device address = %d, instance = %d is mounted\r\n", dev_addr, instance);

  // Interface protocol (hid_interface_protocol_enum_t)
  const char* protocol_str[] = { "None", "Keyboard", "Mouse" };
  uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);

  printf("HID Interface Protocol = %s\r\n", protocol_str[itf_protocol]);

  // By default host stack will use activate boot protocol on supported interface.
  // Therefore for this simple example, we only need to parse generic report descriptor (with built-in parser)
  if ( itf_protocol == HID_ITF_PROTOCOL_NONE )
  {
    hid_info[instance].report_count = tuh_hid_parse_report_descriptor(hid_info[instance].report_info, MAX_REPORT, desc_report, desc_len);
    printf("HID has %u reports \r\n", hid_info[instance].report_count);
  }

  if ( itf_protocol == HID_ITF_PROTOCOL_KEYBOARD )
  {
    hid_info[instance].report_count = tuh_hid_parse_report_descriptor(hid_info[instance].report_info, MAX_REPORT, desc_report, desc_len);
    printf("HID has %u reports \r\n", hid_info[instance].report_count);
    keybd_dev_addr = dev_addr;
  }

  // request to receive report
  // tuh_hid_report_received_cb() will be invoked when report is available
  if ( !tuh_hid_receive_report(dev_addr, instance) )
  {
    printf("Error: cannot request to receive report\r\n");
  }
}

// Invoked when device with hid interface is un-mounted
void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance)
{
  printf("HID device address = %d, instance = %d is unmounted\r\n", dev_addr, instance);

  keybd_dev_addr = 0xff;
}

// Invoked when received report from device via interrupt endpoint
void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
  uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);

  switch (itf_protocol)
  {
    case HID_ITF_PROTOCOL_KEYBOARD:
      TU_LOG2("HID receive boot keyboard report\r\n");
      process_kbd_report( (hid_keyboard_report_t const*) report );
    break;
  }

  // continue to request to receive report
  if ( !tuh_hid_receive_report(dev_addr, instance) )
  {
    printf("Error: cannot request to receive report\r\n");
  }
}

//--------------------------------------------------------------------+
// Keyboard
//--------------------------------------------------------------------+

static void process_kbd_report(hid_keyboard_report_t const *report)
{
  printf("mod:%02x key:%02x %02x %02x %02x %02x %02x\n",
          report->modifier,
          report->keycode[0], report->keycode[1], report->keycode[2],
          report->keycode[3], report->keycode[4], report->keycode[5]);
}
