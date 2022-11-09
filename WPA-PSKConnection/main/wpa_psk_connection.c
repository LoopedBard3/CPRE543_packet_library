#include <string.h>
#include <esp_wifi.h>
#include <esp_task_wdt.h>
#include <esp_log.h>
#include <esp_event.h>
#include <nvs_flash.h>
#include "packet_library.h"

#define ISAP false
#define SSID "esp32SSID"
#define SSIDPASSWORD "esp32test"
#define APSENDBROADCAST true
#define APSENDINDIVIDUAL true

uint8_t mac[6];
bool mac_set;

uint8_t ap_mac[6];
uint8_t sta_macs_holder[10][6];
int connected_stas_count;

// Create the callback we want to use when we get a packet
static void ap_general_callback(wifi_mac_data_frame_t* packet, int payload_length)
{
    if(!mac_set)
    {
        ESP_ERROR_CHECK(get_current_mac(mac));
    }
    if(packet->address_1[0] == mac[0] && packet->address_1[1] == mac[1] && packet->address_1[2] == mac[2] 
    && packet->address_1[3] == mac[3] && packet->address_1[4] == mac[4] && packet->address_1[5] == mac[5])
    {
        ESP_LOGI(LOGGING_TAG, "Packet Received For Us");
        log_packet_annotated(packet, payload_length, LOGGING_TAG);
    }
}

static void sta_general_callback(wifi_mac_data_frame_t* packet, int payload_length)
{
    if(!mac_set)
    {
        ESP_ERROR_CHECK(get_current_mac(mac));
    }
    if(packet->address_1[0] == mac[0] && packet->address_1[1] == mac[1] && packet->address_1[2] == mac[2] 
    && packet->address_1[3] == mac[3] && packet->address_1[4] == mac[4] && packet->address_1[5] == mac[5])
    {
        ESP_LOGI(LOGGING_TAG, "Packet Received To Us");
        log_packet_annotated(packet, payload_length, LOGGING_TAG);
    }

    if(packet->address_1[0] == 0xFF && packet->address_1[1] == 0xFF && packet->address_1[2] == 0xFF 
    && packet->address_1[3] == 0xFF && packet->address_1[4] == 0xFF && packet->address_1[5] == 0xFF && 
    packet->address_2[0] == ap_mac[0] && packet->address_2[1] == ap_mac[1] && packet->address_2[2] == ap_mac[2] 
    && packet->address_2[3] == ap_mac[3] && packet->address_2[4] == ap_mac[4] && packet->address_2[5] == ap_mac[5])
    {
        ESP_LOGI(LOGGING_TAG, "Packet Received Broadcast");
        log_packet_annotated(packet, payload_length, LOGGING_TAG);
    }
}

/* WPA2-PSK connection method */
static void wpa_psk_connection(void)
{
    //ESP_ERROR_CHECK(set_send_post_callback_print(ANNOTATED));
    if(ISAP)
    {
        wifi_ap_config_t ap_config = {
            .ssid = SSID,
            .password = SSIDPASSWORD,
            .ssid_len = 0,
            .channel = 6,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .ssid_hidden =  false,
            .max_connection = 4,
            .beacon_interval = 300
        };
        ESP_ERROR_CHECK(setup_wifi_access_point_simple());
        ESP_ERROR_CHECK(setup_promiscuous_simple_with_general_callback(&ap_general_callback)); // Option to turn on promiscuous listener
        ESP_ERROR_CHECK(setup_wpa_ap(ap_config));
        wifi_promiscuous_filter_t packet_filter = {
            .filter_mask = WIFI_PROMIS_FILTER_MASK_DATA
        };
        ESP_ERROR_CHECK(setup_packets_type_filter(&packet_filter));

        bool packet_sent = false;
        uint8_t payload[8] = { 0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10 };
        int payload_length = 8;
        const TickType_t xDelay = 4000 / portTICK_PERIOD_MS; 
        while(true){
            packet_sent = false;
            if(APSENDINDIVIDUAL)
            {
                ESP_ERROR_CHECK(ap_get_current_connected_sta_macs(sta_macs_holder, &connected_stas_count));
                int counter = 0;
                for(counter = 0; counter < connected_stas_count; counter++)
                {
                    ESP_ERROR_CHECK(ap_send_payload_to_station(payload, payload_length, sta_macs_holder[counter]));
                    packet_sent = true;
                }
            }
            if(APSENDBROADCAST && connected_stas_count > 0)
            {
                packet_sent = true;
                ESP_ERROR_CHECK(ap_send_payload_to_all_stations(payload, payload_length));
            }
            if(packet_sent)
            {
                ESP_LOGI(LOGGING_TAG, "AP PACKET(S) SENT");
            }
            vTaskDelay( xDelay );
        }
    }

    if(!ISAP)
    {
        wifi_sta_config_t sta_config = {
            .ssid = SSID,
            .password = SSIDPASSWORD,
            .scan_method = WIFI_FAST_SCAN,
            .bssid_set = 0,
            .channel = 0, // Use channel 0 to try to find channel
        };
        ESP_ERROR_CHECK(setup_wifi_station_simple());
        ESP_ERROR_CHECK(setup_promiscuous_simple_with_general_callback(&sta_general_callback)); // Option to turn on promiscuous listener
        ESP_ERROR_CHECK(setup_wpa_sta(sta_config));
        ESP_ERROR_CHECK(get_current_ap_mac(ap_mac));
        wifi_promiscuous_filter_t packet_filter = {
            .filter_mask = WIFI_PROMIS_FILTER_MASK_DATA
        };
        ESP_ERROR_CHECK(setup_packets_type_filter(&packet_filter));

        uint8_t payload[8] = { 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF };
        int payload_length = 8;
        const TickType_t xDelay = 4000 / portTICK_PERIOD_MS; 
        while(true){
            ESP_ERROR_CHECK(sta_send_payload_to_access_point(payload, payload_length));
            ESP_LOGI(LOGGING_TAG, "STATION PACKET SENT");
            vTaskDelay( xDelay );
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
    wpa_psk_connection();
}
