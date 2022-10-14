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
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "packet_library.h"

#define SENDER false
#define RECEIVER true

/* Library test method */
static void library_test(void)
{
    setup_wifi_simple();

    if(RECEIVER)// TODO for library
    {
        ESP_ERROR_CHECK(set_receive_pre_callback_print(ANNOTATED));
        ESP_ERROR_CHECK(set_receive_post_callback_print(HEX));
        wifi_promiscuous_filter_t packet_filter = {
            .filter_mask = WIFI_PROMIS_FILTER_MASK_DATA
        };
        ESP_ERROR_CHECK(setup_packets_type_filter(&packet_filter));
        // TODO Individual field callbacks and general callback
        ESP_ERROR_CHECK(setup_promiscuous_simple());
    }
    if(SENDER)// TODO
    {
        setup_sta_default();
        wifi_mac_data_frame_t* pkt_format = calloc(1, sizeof(wifi_mac_data_frame_t) + 8);
        
        pkt_format->frame_control = 0b10000000;
        pkt_format->duration_id = 0;
        uint8_t add1[6] = { 0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5 };
        memcpy(pkt_format->address_1, add1, sizeof(add1));
        uint8_t add2[6] = { 0xD0, 0xD2, 0xD2, 0xD3, 0xD4, 0xD5 };
        memcpy(pkt_format->address_2, add2, sizeof(add2));
        uint8_t add3[6] = { 0xD0, 0xD3, 0xD2, 0xD3, 0xD4, 0xD5 };
        memcpy(pkt_format->address_3, add3, sizeof(add3));
        pkt_format->sequence_control = 0;
        uint8_t add4[6] = { 0xD0, 0xD4, 0xD2, 0xD3, 0xD4, 0xD5 };
        memcpy(pkt_format->address_4, add4, sizeof(add4));
        strcpy((char *)pkt_format->payload, "TESTING");
        int length = sizeof(wifi_mac_data_frame_t) + 8;
        uint8_t mac[6];
        ESP_ERROR_CHECK(esp_wifi_get_mac(WIFI_IF_STA, mac));
        while(true){
            esp_wifi_80211_tx(WIFI_IF_STA, (void *)&pkt_format, length, false);
            ESP_LOGI(LOGGING_TAG, "PACKET SENT");
            ESP_LOGI(LOGGING_TAG, "MAC: %02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        }
    }
}

void app_main(void)
{
    // Initialize NVS (Not part of wifi library functionality)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    //esp_log_level_set(LOGGING_TAG, ESP_LOG_ERROR);
    library_test();
}
