/* Scan Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

/*
    This example shows how to scan for available set of APs.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "regex.h"
#include <stdio.h>
#include <string.h>
#include "freertos/task.h"
#include "esp_http_server.h"
#include "esp_ota_ops.h"
#include "esp_netif.h"
#include "driver/gpio.h"
// #include "esp_tinyusb.h"
//#include "tusb_hid.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "esp_err.h"
#include "esp_log.h"
#include "usb/usb_host.h"
#include "errno.h"
#include "driver/gpio.h"

#include "usb/hid_host.h"
#include "usb/hid_usage_keyboard.h"
// #include "usb/hid_usage_mouse.h"

#define TAG "Joystick OTA"

// Wi-Fi Credentials
#define WIFI_SSID "XCREMOTE"
#define WIFI_PASS "xcremote"

// GPIO Pins
#define HAT_UP_PIN GPIO_NUM_18
#define HAT_LEFT_PIN GPIO_NUM_37
#define HAT_DOWN_PIN GPIO_NUM_38
#define HAT_RIGHT_PIN GPIO_NUM_39
#define HAT_CENTER_PIN GPIO_NUM_15
#define RECTANGLE_BUTTON_PIN GPIO_NUM_7
#define CIRCLE_BUTTON_PIN GPIO_NUM_42
#define CANCEL_BUTTON_PIN GPIO_NUM_45
#define TRIANGLE_BUTTON_PIN GPIO_NUM_0
#define PTT_BUTTON_PIN GPIO_NUM_41

// HID Keyboard Keycodes
#define KEY_ARROW_UP 0x52
#define KEY_ARROW_DOWN 0x51
#define KEY_ARROW_LEFT 0x50
#define KEY_ARROW_RIGHT 0x4F
#define KEY_ENTER 0x28
#define KEY_A 0x04
#define KEY_B 0x05
#define KEY_C 0x06
#define KEY_D 0x07
#define KEY_E 0x08

// Button Press Durations
#define LONG_PRESS_DURATION_MS 1000
#define SHORT_PRESS_DURATION_MS 200

// USB HID Gamepad Report
static uint8_t gamepad_report[8];

// OTA Update Variables
esp_ota_handle_t ota_handle;
const esp_partition_t *ota_partition = NULL;

// HTTP Server
static httpd_handle_t server = NULL;

// Button states
static bool button_pressed = false;
static uint32_t button_press_start_time = 0;

// Static Files
extern const unsigned char jquery_min_js_start[] asm("_binary_jquery_min_js_start");
extern const unsigned char jquery_min_js_end[] asm("_binary_jquery_min_js_end");
size_t jquery_min_js_len =2048;// (jquery_min_js_end - jquery_min_js_start);

// HID Key Values
enum
{
    HAT_CENTER = 0,
    HAT_UP = 1,
    HAT_RIGHT = 2,
    HAT_DOWN = 3,
    HAT_LEFT = 4,
    HAT_UP_RIGHT = 5,
    HAT_DOWN_RIGHT = 6,
    HAT_DOWN_LEFT = 7,
    HAT_UP_LEFT = 8
};

// Function Declarations
void init_gpio(void);
void start_wifi_ap(void);
void start_http_server(void);
void handle_buttons(void *arg);
void update_hat_switch(void);
void start_tinyusb(void);
void send_gamepad_report(void);
static void hid_host_task(void *arg);
static void usb_host_event_callback(const usb_host_client_event_msg_t *event_msg, void *arg);
static void send_keypress(uint8_t keycode);
static void send_keyrelease(void);
static void monitor_gpio_task(void *arg);
// USB HID Host Setup
usb_host_client_handle_t client_hdl = NULL;
usb_device_handle_t device_hdl = NULL;

// Send Keypress
static void send_keypress(uint8_t keycode)
{
    uint8_t report[8] = {0};               // HID report: [Modifiers, Reserved, Keycode1, Keycode2...Keycode6]
    report[2] = keycode;                   // Set the keycode to send
    //tud_hid_keyboard_report(0, 0, report); // Send HID report
    ESP_LOGI(TAG, "Key pressed: 0x%02X", keycode);
}

// Send Key Release
static void send_keyrelease(void)
{
    uint8_t report[8] = {0}; // Empty report to indicate key release
    //tud_hid_keyboard_report(0, 0, report);
    ESP_LOGI(TAG, "Key released");
}

// Monitor GPIOs and Emulate Keypresses
static void monitor_gpio_task(void *arg)
{
    while (1)
    {
        // Check each GPIO and send the corresponding keypress
        if (gpio_get_level(HAT_UP_PIN) == 0)
        {
            send_keypress(KEY_ARROW_UP);
        }
        else if (gpio_get_level(HAT_DOWN_PIN) == 0)
        {
            send_keypress(KEY_ARROW_DOWN);
        }
        else if (gpio_get_level(HAT_LEFT_PIN) == 0)
        {
            send_keypress(KEY_ARROW_LEFT);
        }
        else if (gpio_get_level(HAT_RIGHT_PIN) == 0)
        {
            send_keypress(KEY_ARROW_RIGHT);
        }
        else if (gpio_get_level(HAT_CENTER_PIN) == 0)
        {
            send_keypress(KEY_ENTER);
        }
        else if (gpio_get_level(RECTANGLE_BUTTON_PIN) == 0)
        {
            send_keypress(KEY_A);
        }
        else if (gpio_get_level(CIRCLE_BUTTON_PIN) == 0)
        {
            send_keypress(KEY_B);
        }
        else if (gpio_get_level(CANCEL_BUTTON_PIN) == 0)
        {
            send_keypress(KEY_C);
        }
        else if (gpio_get_level(TRIANGLE_BUTTON_PIN) == 0)
        {
            send_keypress(KEY_D);
        }
        else if (gpio_get_level(PTT_BUTTON_PIN) == 0)
        {
            send_keypress(KEY_E);
        }
        else
        {
            // No button pressed, send key release
            send_keyrelease();
        }

        vTaskDelay(pdMS_TO_TICKS(10)); // Poll every 10ms
    }
}
// USB Host Event Callback
static void usb_host_event_callback(const usb_host_client_event_msg_t *event_msg, void *arg)
{
    if (event_msg->event == USB_HOST_CLIENT_EVENT_NEW_DEV)
    {
        device_hdl = event_msg->new_dev.device_handle;
        ESP_LOGI(TAG, "New device connected.");
    }
    else if (event_msg->event == USB_HOST_CLIENT_EVENT_DEV_GONE)
    {
        ESP_LOGI(TAG, "Device disconnected.");
        device_hdl = NULL;
    }
}

// USB Host Task
static void hid_host_task(void *arg)
{
    usb_host_client_config_t client_config = {
        .is_synchronous = false,
    };
    const usb_host_config_t host_config = {
        .skip_phy_setup = false,
        .intr_flags = ESP_INTR_FLAG_LEVEL1,
    };
    usb_host_install(&host_config);
    usb_host_client_register(&client_config, &client_hdl);

    while (1)
    {
        if (device_hdl)
        {
            // Handle HID events here
            ESP_LOGI(TAG, "Processing HID device...");
            handle_buttons(NULL);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// HTTP Handlers
esp_err_t handle_index(httpd_req_t *req)
{
    const char *index_html =
        "<script src='/jquery.min.js'></script>"
        "<h3>XCREMOTE Updater</h3>"
        "<form method='POST' action='/update' enctype='multipart/form-data'>"
        "<input type='file' name='update' style='width:600px'><br><br>"
        "<input type='submit' value='Update'>"
        "</form>";
    httpd_resp_send(req, index_html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t handle_jquery(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/javascript");
    httpd_resp_send(req, (const char *)jquery_min_js_start, jquery_min_js_len);
    return ESP_OK;
}

esp_err_t handle_update(httpd_req_t *req)
{
    ota_partition = esp_ota_get_next_update_partition(NULL);
    if (!ota_partition)
    {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No OTA partition found.");
        return ESP_FAIL;
    }
    esp_ota_begin(ota_partition, OTA_SIZE_UNKNOWN, &ota_handle);

    char buf[1024];
    int remaining = req->content_len;
    int bytes_received;
    while (remaining > 0)
    {
        bytes_received = httpd_req_recv(req, buf, sizeof(buf));
        if (bytes_received <= 0)
        {
            esp_ota_end(ota_handle);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA failed.");
            return ESP_FAIL;
        }
        esp_ota_write(ota_handle, buf, bytes_received);
        remaining -= bytes_received;
    }

    esp_ota_end(ota_handle);
    esp_ota_set_boot_partition(ota_partition);
    httpd_resp_sendstr(req, "OTA Update Complete. Rebooting...");
    esp_restart();
    return ESP_OK;
}

void start_http_server(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_start(&server, &config);

    httpd_uri_t uri_index = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = handle_index};
    httpd_register_uri_handler(server, &uri_index);

    httpd_uri_t uri_jquery = {
        .uri = "/jquery.min.js",
        .method = HTTP_GET,
        .handler = handle_jquery};
    httpd_register_uri_handler(server, &uri_jquery);

    httpd_uri_t uri_update = {
        .uri = "/update",
        .method = HTTP_POST,
        .handler = handle_update};
    httpd_register_uri_handler(server, &uri_update);
}

void start_wifi_ap(void)
{
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t ap_config = {
        .ap = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .ssid_len = strlen(WIFI_SSID),
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA2_PSK}};
    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config);
    esp_wifi_start();
    ESP_LOGI(TAG, "Wi-Fi Access Point started. SSID: %s", WIFI_SSID);
}

void init_gpio(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << HAT_UP_PIN) |
                        (1ULL << HAT_DOWN_PIN) |
                        (1ULL << HAT_LEFT_PIN) |
                        (1ULL << HAT_RIGHT_PIN) |
                        (1ULL << RECTANGLE_BUTTON_PIN) |
                        (1ULL << CIRCLE_BUTTON_PIN) |
                        (1ULL << CANCEL_BUTTON_PIN) |
                        (1ULL << TRIANGLE_BUTTON_PIN) |
                        (1ULL << PTT_BUTTON_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&io_conf);
}

void handle_buttons(void *arg)
{
    while (1)
    {
        // Button handling logic
        update_hat_switch();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void update_hat_switch(void)
{
    uint8_t hat_position = HAT_CENTER;

    if (gpio_get_level(HAT_UP_PIN) == 0)
        hat_position = HAT_UP;
    else if (gpio_get_level(HAT_DOWN_PIN) == 0)
        hat_position = HAT_DOWN;
    else if (gpio_get_level(HAT_LEFT_PIN) == 0)
        hat_position = HAT_LEFT;
    else if (gpio_get_level(HAT_RIGHT_PIN) == 0)
        hat_position = HAT_RIGHT;

    gamepad_report[0] = hat_position; // Update the hat switch position
                                      // send_gamepad_report();
}

// void start_tinyusb(void)
// {
//     const tinyusb_config_t tusb_cfg = {};
//     ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
//     ESP_LOGI(TAG, "TinyUSB HID started.");
// }

void send_gamepad_report(void)
{
    tud_hid_report(0, gamepad_report, sizeof(gamepad_report));
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting Joystick OTA...");
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    init_gpio();
    start_wifi_ap();
    start_http_server();
    start_tinyusb();
    // Start GPIO Monitoring Task
    xTaskCreate(monitor_gpio_task, "monitor_gpio_task", 2048, NULL, 5, NULL);
    //  xTaskCreate(handle_buttons, "button_task", 2048, NULL, 5, NULL);
    // Start HID Host Task
    // xTaskCreate(hid_host_task, "hid_host_task", USB_HOST_TASK_STACK_SIZE, NULL, USB_HOST_TASK_PRIORITY, NULL);
}
