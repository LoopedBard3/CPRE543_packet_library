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


//static const char *TAG = "scan";

// static void promisc_callback(void *buf, wifi_promiscuous_pkt_type_t type)
// {
//     wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
//     int length = pkt->rx_ctrl.sig_len;
//     wifi_mac_data_frame_t *frame = (wifi_mac_data_frame_t *)pkt->payload;
//     printf("Printing inside callback.");
//     if(type == WIFI_PKT_MGMT)
//     {
//         ESP_LOGI(TAG, "Management packet, Length: %d, First 64: %" PRIu64 "", length, (uint64_t) pkt->payload);
//     }
//     else if(type == WIFI_PKT_CTRL)
//     { 
//         ESP_LOGI(TAG, "Control packet, Length: %d, First 64: %" PRIu64 "", length, (uint64_t) pkt->payload);        
//     }
//     else if(type == WIFI_PKT_DATA)
//     { 
//         ESP_LOGI(TAG, "Data packet, Length: %d, First 64: %" PRIu64 "", length, (uint64_t) pkt->payload);        
//     }
//     else if(type == WIFI_PKT_MISC)
//     { 
//         ESP_LOGI(TAG, "Miscellaneous packet, Length: %d, First 64: %" PRIu64 "", length, (uint64_t) pkt->payload);        
//     }
//     else
//     { 
//         ESP_LOGI(TAG, "Unknown packet type, Length: %d", length);        
//     }
    
//     ESP_LOGI(TAG, "MAIN: Frame Control: %hhu, Packet Addr1: %02x:%02x:%02x:%02x:%02x:%02x, Addr2: %02x:%02x:%02x:%02x:%02x:%02x, Addr3 %02x:%02x:%02x:%02x:%02x:%02x, Addr4 %02x:%02x:%02x:%02x:%02x:%02x.", frame->frame_control,
//         frame->address_1[0], frame->address_1[1], frame->address_1[2], frame->address_1[3], frame->address_1[4], frame->address_1[5],
//         frame->address_2[0], frame->address_2[1], frame->address_2[2], frame->address_2[3], frame->address_2[4], frame->address_2[5],
//         frame->address_3[0], frame->address_3[1], frame->address_3[2], frame->address_3[3], frame->address_3[4], frame->address_3[5],
//         frame->address_4[0], frame->address_4[1], frame->address_4[2], frame->address_4[3], frame->address_4[4], frame->address_4[5]
//     );
//     ESP_LOGI(TAG, "Annotated Packet Format:");
//     log_packet_annotated(frame, length - sizeof(wifi_mac_data_frame_t) - sizeof(uint8_t), TAG);    
// }

/* Library test method */
static void library_test(void)
{
    setup_wifi_simple();

    if(RECEIVER)// TODO for library
    {
        ESP_ERROR_CHECK(set_receive_pre_callback_print(ANNOTATED));
        ESP_ERROR_CHECK(set_receive_post_callback_print(ANNOTATED));
        // TODO Individual field callbacks and general callback
        ESP_ERROR_CHECK(setup_promiscuous_simple());
    }
    if(SENDER)// TODO
    {
        setup_sta_default();
        wifi_mac_data_frame_t pkt_format = {
            .frame_control = 0b10000000,
            .duration_id = 0,
            .address_1 = { 0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5 },
            .address_2 = { 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB },
            .address_3 = { 0xDC, 0xDD, 0xDE, 0xDF, 0xC0, 0xC1 },
            .sequence_control = 0,
            .address_4 = { 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7 },
            .payload = (uint8_t *)"TESTING" 
        };
        int length = sizeof(pkt_format);
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

    library_test();
}
