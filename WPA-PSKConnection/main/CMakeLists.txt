idf_component_register(SRCS "wpa_psk_connection.c"
                    INCLUDE_DIRS "."
                    REQUIRES packet_library esp_wifi nvs_flash)
