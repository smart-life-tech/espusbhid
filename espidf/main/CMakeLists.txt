#cmake_minimum_required(VERSION 3.24)
idf_component_register(SRCS "scan.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_tinyusb
                    PRIV_REQUIRES esp_wifi nvs_flash esp_http_server app_update esp_netif)
