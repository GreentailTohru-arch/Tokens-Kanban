idf_component_register(
    SRCS "main.c" "token_model.c" "token_ui.c" "token_ble.c" "postcard.c" "postcard_codec.c" "../assets/fonts/token_font_14.c" "../assets/images/postcard_default_rgb565.c"
    INCLUDE_DIRS "."
    REQUIRES bsp esp_timer esp_driver_usb_serial_jtag bt nvs_flash esp_partition
)
