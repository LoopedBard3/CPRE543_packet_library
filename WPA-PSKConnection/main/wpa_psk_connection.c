#include <string.h>
#include <esp_wifi.h>
#include <esp_task_wdt.h>
#include <esp_log.h>
#include <esp_event.h>
#include <nvs_flash.h>
#include "packet_library.h"


// Important Note: The access point ESP-32 has to be turned on and flashed before the stations in order for the stations to make a connection.
#define ISAP false // Bool to specify if the flashed ESP-32 should run as the AP or a station
#define SSID "esp32SSID" // The name the AP will broadcast as
#define SSIDPASSWORD "esp32test" // The password to connect to the AP
#define APSENDBROADCAST true // Bool to specify whether the AP should occassionally send out broadcast packets
#define APSENDINDIVIDUAL true // Bool to specify whether the AP should occassionally send out individual packets to each station

// Variables used in the code, no changes necessary to work
bool mac_set;
uint8_t mac[6];
uint8_t ap_mac[6];
uint8_t sta_macs_holder[10][6];
int connected_stas_count;

// Create the callback for packets received by the AP
static void ap_general_callback(wifi_mac_data_frame_t* packet, int payload_length)
{
    // Set the mac address holder if we have not set it yet
    if(!mac_set)
    {
        ESP_ERROR_CHECK(get_current_mac(mac));
        mac_set = true;
    }

    // If the packet is to the AP, log the packet with annotations
    if(packet->address_1[0] == mac[0] && packet->address_1[1] == mac[1] && packet->address_1[2] == mac[2] 
    && packet->address_1[3] == mac[3] && packet->address_1[4] == mac[4] && packet->address_1[5] == mac[5])
    {
        ESP_LOGI(LOGGING_TAG, "Packet Received For Us");
        log_packet_annotated(packet, payload_length, LOGGING_TAG);
    }
}

// Create the callback for packets received by the stations
static void sta_general_callback(wifi_mac_data_frame_t* packet, int payload_length)
{
    // Set the mac address holder if we have not set it yet
    if(!mac_set)
    {
        ESP_ERROR_CHECK(get_current_mac(mac));
        mac_set = true;
    }

    // If the packet is to us, log the packet with annotations
    if(packet->address_1[0] == mac[0] && packet->address_1[1] == mac[1] && packet->address_1[2] == mac[2] 
    && packet->address_1[3] == mac[3] && packet->address_1[4] == mac[4] && packet->address_1[5] == mac[5])
    {
        ESP_LOGI(LOGGING_TAG, "Packet Received To Us");
        log_packet_annotated(packet, payload_length, LOGGING_TAG);
    }

    // If the packet is a broadcast from the AP, log the packet with annotations
    if(packet->address_1[0] == 0xFF && packet->address_1[1] == 0xFF && packet->address_1[2] == 0xFF 
    && packet->address_1[3] == 0xFF && packet->address_1[4] == 0xFF && packet->address_1[5] == 0xFF && 
    packet->address_2[0] == ap_mac[0] && packet->address_2[1] == ap_mac[1] && packet->address_2[2] == ap_mac[2] 
    && packet->address_2[3] == ap_mac[3] && packet->address_2[4] == ap_mac[4] && packet->address_2[5] == ap_mac[5])
    {
        ESP_LOGI(LOGGING_TAG, "Packet Received Broadcast");
        log_packet_annotated(packet, payload_length, LOGGING_TAG);
    }
}

// WPA2-PSK connection method, the primary running code
static void wpa_psk_connection(void)
{
    // If this ESP-32 should run as an AP
    if(ISAP)
    {
        // Create the AP configuration, setting the SSID and password from the ones defined above. 
        // More information available: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s2/api-reference/network/esp_wifi.html#_CPPv413wifi_config_t
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
        // Setup the ESP-32's wifi configuration to act as an AP, callback, and callback filter
        ESP_ERROR_CHECK(setup_wifi_access_point_simple());
        ESP_ERROR_CHECK(setup_promiscuous_simple_with_general_callback(&ap_general_callback)); // Option to turn on promiscuous listener
        ESP_ERROR_CHECK(setup_wpa_ap(ap_config)); // Use the generated AP config to setup the device as an AP
        wifi_promiscuous_filter_t packet_filter = {
            .filter_mask = WIFI_PROMIS_FILTER_MASK_DATA
        };
        ESP_ERROR_CHECK(setup_packets_type_filter(&packet_filter));

        // Setup loop to send a packet to the AP every 4 seconds
        bool packet_sent = false;
        uint8_t payload[8] = { 0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10 };
        int payload_length = 8;
        const TickType_t xDelay = 4000 / portTICK_PERIOD_MS; 
        while(true){
            packet_sent = false;
            // If individual packet sending is enabled, send them
            if(APSENDINDIVIDUAL)
            {
                ESP_ERROR_CHECK(ap_get_current_connected_sta_macs(sta_macs_holder, &connected_stas_count)); // Get the list of the currently connected stations MAC addresses
                int counter = 0;
                for(counter = 0; counter < connected_stas_count; counter++)
                {
                    // For each connected station, send it a payload
                    ESP_ERROR_CHECK(ap_send_payload_to_station(payload, payload_length, sta_macs_holder[counter]));
                    packet_sent = true;
                }
            }
            // If broadcast packet sending is enabled, send it
            if(APSENDBROADCAST && connected_stas_count > 0)
            {
                packet_sent = true;
                ESP_ERROR_CHECK(ap_send_payload_to_all_stations(payload, payload_length)); // This sends the payload as a broadcast packet (target address 0xFF:FF:FF:FF:FF:FF)
            }
            // If a packet is sent, log that we sent something
            if(packet_sent)
            {
                ESP_LOGI(LOGGING_TAG, "AP PACKET(S) SENT");
            }
            vTaskDelay( xDelay );
        }
    }

    // If the ESP-32 should run as a station
    if(!ISAP)
    {
        // Create the station configuration, setting the SSID and password to the ones defined above so this ESP-32 can connect to it.
        // More information available: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s2/api-reference/network/esp_wifi.html#_CPPv413wifi_config_t
        wifi_sta_config_t sta_config = {
            .ssid = SSID,
            .password = SSIDPASSWORD,
            .scan_method = WIFI_FAST_SCAN,
            .bssid_set = 0,
            .channel = 0, // Use channel 0 to dynamically find the channel
        };

        // Setup the ESP-32's wifi configuration to act as a station, callback, and callback filter
        ESP_ERROR_CHECK(setup_wifi_station_simple());
        ESP_ERROR_CHECK(setup_promiscuous_simple_with_general_callback(&sta_general_callback)); // Option to turn on promiscuous listener
        ESP_ERROR_CHECK(setup_wpa_sta(sta_config)); // Use the generated station config to setup the device as an station
        wifi_promiscuous_filter_t packet_filter = {
            .filter_mask = WIFI_PROMIS_FILTER_MASK_DATA
        };
        ESP_ERROR_CHECK(setup_packets_type_filter(&packet_filter));
        
        // Get the mac address of the AP
        ESP_ERROR_CHECK(get_current_ap_mac(ap_mac));

        // Setup loop to send a packet to the AP every 4 seconds
        uint8_t payload[8] = { 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF };
        int payload_length = 8;
        const TickType_t xDelay = 4000 / portTICK_PERIOD_MS; 
        while(true){
            // Actually send the packet
            ESP_ERROR_CHECK(sta_send_payload_to_access_point(payload, payload_length));
            ESP_LOGI(LOGGING_TAG, "STATION PACKET SENT");
            // Wait 4000ms
            vTaskDelay( xDelay );
        }
    }
}

void app_main(void)
{
    // Initialize NVS (Not part of the packet component functionality)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    // Call the primary method
    wpa_psk_connection();
}
