idf_component_register(SRCS "dashboard_main.c"
                           "lvgl/lvgl_setup.c"
                           "lvgl/system_monitor_ui.c"
                           "serial/serial_data_handler.c"
                           "touch/gt911_touch.c"
                           "wifi/wifi_manager.c"
                           "smart/home_assistant_api.c"
                       INCLUDE_DIRS "." "lvgl" "serial" "touch" "wifi" "smart"
                       REQUIRES lvgl__lvgl esp_lcd driver json esp_wifi esp_netif esp_http_client nvs_flash)
