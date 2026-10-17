set(management_c "${CMAKE_CURRENT_BINARY_DIR}/management_assets.c")
if(NOT CMAKE_BUILD_EARLY_EXPANSION)
file(GLOB_RECURSE management_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../companion/postcard/*")
add_custom_command(OUTPUT "${management_c}"
    COMMAND ${PYTHON} "${CMAKE_CURRENT_SOURCE_DIR}/../tools/embed_management.py" "${management_c}"
    DEPENDS ${management_sources} "../tools/embed_management.py" "../companion/postcard_server.py"
    VERBATIM)
endif()
idf_component_register(
    SRCS "${management_c}" "network_model.c" "network_http.c" "token_wifi.c" "main.c" "boot_logo.c" "boot_logo_model.c" "device_diagnostics.c" "diagnostic_model.c" "device_model.c" "device_runtime.c" "token_model.c" "page_model.c" "volume_model.c" "system_model.c" "monitor_model.c" "../assets/fonts/token_text_coverage.c" "token_ui.c" "token_ble.c" "postcard.c" "postcard_codec.c" "../assets/fonts/token_font_14.c" "../assets/images/postcard_default_rgb565.c" "../assets/music/codex_sounds.c" "../assets/images/startup_assets.c"
    INCLUDE_DIRS "."
    REQUIRES esp_wifi esp_netif esp_event esp_http_server lwip mbedtls bsp esp_timer esp_driver_usb_serial_jtag bt nvs_flash esp_partition esp_app_format esp_hw_support spi_flash
)
