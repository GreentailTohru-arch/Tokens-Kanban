idf_component_register(
    SRCS "main.c" "boot_logo.c" "boot_logo_model.c" "device_diagnostics.c" "diagnostic_model.c" "device_model.c" "device_runtime.c" "token_model.c" "page_model.c" "volume_model.c" "system_model.c" "monitor_model.c" "../assets/fonts/token_text_coverage.c" "token_ui.c" "token_ble.c" "postcard.c" "postcard_codec.c" "../assets/fonts/token_font_14.c" "../assets/images/postcard_default_rgb565.c" "../assets/music/codex_sounds.c" "../assets/images/startup_assets.c"
    INCLUDE_DIRS "."
    REQUIRES bsp esp_timer esp_driver_usb_serial_jtag bt nvs_flash esp_partition esp_app_format esp_hw_support spi_flash
)
