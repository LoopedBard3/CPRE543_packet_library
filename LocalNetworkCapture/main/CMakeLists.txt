idf_component_register(SRCS "local_network_capture.c"
                    INCLUDE_DIRS "."
                    REQUIRES packet_library esp_wifi nvs_flash)
