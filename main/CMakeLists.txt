idf_component_register(SRCS "dashboard_main.c"
                           "lvgl/lvgl_setup.c"
                           "lvgl/simple_ui.c"
                           "lvgl/system_monitor_ui.c"
                           "serial/serial_data_handler.c"
                       INCLUDE_DIRS "." "lvgl" "serial"
                       REQUIRES lvgl__lvgl esp_lcd driver json)
