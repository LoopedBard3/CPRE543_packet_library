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
#define DO_GENERAL_CALLBACK_TEST true
#define DO_INDIVIDUAL_CALLBACK_TEST true

static void double_general_callback(wifi_mac_data_frame_t* packet)
{
    packet->duration_id = packet->duration_id * 2;
    ESP_LOGI(LOGGING_TAG, "CB: G, O DI: %d", packet->duration_id);
}
static void double_frame_control_callback(uint16_t* frame_control)
{
    *frame_control = *frame_control * 2;
    ESP_LOGI(LOGGING_TAG, "CB: FC, NV: %04X", *frame_control);
}
static void double_duration_id_callback(uint16_t* duration_id)
{
    *duration_id = *duration_id * 2;    
    ESP_LOGI(LOGGING_TAG, "CB: DI, NV: %04X", *duration_id);
}
static void double_address_1_callback(uint8_t address_1[6])
{
    address_1[1] = address_1[0] * 2;    
    ESP_LOGI(LOGGING_TAG, "CB: A1, NV: %02X", address_1[1]);
}
static void double_address_2_callback(uint8_t address_2[6])
{
    address_2[1] = address_2[0] * 2;    
    ESP_LOGI(LOGGING_TAG, "CB: A2, NV: %02X", address_2[1]);
}
static void double_address_3_callback(uint8_t address_3[6])
{
    address_3[1] = address_3[0] * 2;    
    ESP_LOGI(LOGGING_TAG, "CB: A3, NV: %02X", address_3[1]);
}
static void double_sequence_control_callback(uint16_t* sequence_control)
{
    *sequence_control = *sequence_control * 2;    
    ESP_LOGI(LOGGING_TAG, "CB: SC, NV: %04X", *sequence_control);
}
static void double_address_4_callback(uint8_t address_4[6])
{
    address_4[1] = address_4[0] * 2;    
    ESP_LOGI(LOGGING_TAG, "CB: A4, NV: %02X", address_4[1]);
}
static void promisc_payload_callback(uint8_t payload[], int payload_length)
{
    if(payload_length > 10)
    {
        payload[3] = 255;
    }
    ESP_LOGI(LOGGING_TAG, "CB: P; L: %d", payload_length);
}

/* Library test method */
static void library_test(void)
{
    setup_wifi_simple();

    if(RECEIVER) // TODO for library
    {
        ESP_ERROR_CHECK(set_receive_pre_callback_print(ANNOTATED));
        ESP_ERROR_CHECK(set_receive_post_callback_print(ANNOTATED));
        wifi_promiscuous_filter_t packet_filter = {
            .filter_mask = WIFI_PROMIS_FILTER_MASK_DATA
        };
        ESP_ERROR_CHECK(setup_packets_type_filter(&packet_filter));
        if(DO_INDIVIDUAL_CALLBACK_TEST)
        {
            ESP_ERROR_CHECK(set_receive_callback_frame_control(&double_frame_control_callback));
            ESP_ERROR_CHECK(set_receive_callback_duration_id(&double_duration_id_callback));
            ESP_ERROR_CHECK(set_receive_callback_address_1(&double_address_1_callback));
            ESP_ERROR_CHECK(set_receive_callback_address_2(&double_address_2_callback));
            ESP_ERROR_CHECK(set_receive_callback_address_3(&double_address_3_callback));
            ESP_ERROR_CHECK(set_receive_callback_address_4(&double_address_4_callback));
            ESP_ERROR_CHECK(set_receive_callback_sequence_control(&double_sequence_control_callback));
            ESP_ERROR_CHECK(set_receive_callback_payload(&promisc_payload_callback));
        }

        if(DO_GENERAL_CALLBACK_TEST)
        {
            ESP_ERROR_CHECK(setup_promiscuous_simple_with_general_callback(&double_general_callback));
        }
        else
        {
            ESP_ERROR_CHECK(setup_promiscuous_simple());
        }
    }
    if(SENDER) // TODO
    {
        setup_sta_default();
        wifi_mac_data_frame_t* pkt_format = calloc(1, sizeof(wifi_mac_data_frame_t) + 8);
        
        pkt_format->frame_control = 0b10000000;
        pkt_format->duration_id = 2;
        uint8_t add1[6] = { 0x10, 0x11, 0x12, 0x13, 0x14, 0x15 };
        memcpy(pkt_format->address_1, add1, sizeof(add1));
        uint8_t add2[6] = { 0x10, 0x12, 0x12, 0x13, 0x14, 0x15 };
        memcpy(pkt_format->address_2, add2, sizeof(add2));
        uint8_t add3[6] = { 0x10, 0x13, 0x12, 0x13, 0x14, 0x15 };
        memcpy(pkt_format->address_3, add3, sizeof(add3));
        pkt_format->sequence_control = 0;
        uint8_t add4[6] = { 0x10, 0x14, 0x12, 0x13, 0x14, 0x15 };
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
