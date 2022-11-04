/* Local Network Scan Example
    This demonstrates    
*/

#include <string.h>
#include <esp_wifi.h>
#include <esp_task_wdt.h>
#include <esp_log.h>
#include <esp_event.h>
#include <nvs_flash.h>
#include "packet_library.h"

// Callback helper method that prints out the packets we capture with annotations (this is the inner-code from log_packet_annotated)
esp_err_t log_captured_packet(wifi_mac_data_frame_t* packet, int payload_length, const char * TAG)
{
    ESP_LOGI(TAG, "Frame Control: %04X\nDuration_ID: %04X\nPacket Addr1: %02X:%02X:%02X:%02X:%02X:%02X\nAddr2: %02X:%02X:%02X:%02X:%02X:%02X\nAddr3 %02X:%02X:%02X:%02X:%02X:%02X\nSequence Control: %04X\nAddr4 %02X:%02X:%02X:%02X:%02X:%02X",
        packet->frame_control, packet->duration_id,
        packet->address_1[0], packet->address_1[1], packet->address_1[2], packet->address_1[3], packet->address_1[4], packet->address_1[5],
        packet->address_2[0], packet->address_2[1], packet->address_2[2], packet->address_2[3], packet->address_2[4], packet->address_2[5],
        packet->address_3[0], packet->address_3[1], packet->address_3[2], packet->address_3[3], packet->address_3[4], packet->address_3[5],
        packet->sequence_control,
        packet->address_4[0], packet->address_4[1], packet->address_4[2], packet->address_4[3], packet->address_4[4], packet->address_4[5]
    );

    if(payload_length){
        uint8_t* payload_buffer = (uint8_t*)malloc(payload_length*2); // Times 2 for 2 hex per byte
        char temp_buffer[3];
        // // Generate the payload string
        for(int counter = 0; counter < payload_length; counter++)
        {
            snprintf(temp_buffer, 3, "%02X", (packet->payload)[counter]);
            memcpy(&payload_buffer[counter*2], temp_buffer, 2);
        }

        ESP_LOGI(TAG, "Payload: 0x%.*s", payload_length*2, payload_buffer);
        free(payload_buffer);
    }
    return ESP_OK;    
}

// Create the callback we want to use when we get a packet
static void scan_general_callback(wifi_mac_data_frame_t* packet, int payload_length)
{
    log_captured_packet(packet, payload_length, LOGGING_TAG);
}

/* Local Network Scan method */
static void scan_local_network(void)
{
    // Setup with default WiFi setup
    setup_wifi_simple();

    // Setup the esp32 in prmiscuous mode and enable the general packet callback method implemented above
    ESP_ERROR_CHECK(setup_promiscuous_simple_with_general_callback(&scan_general_callback));
}

void app_main(void)
{
    // Initialize NVS (Not part of wifi library functionality)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    // Make sure nvs got setup properly
    ESP_ERROR_CHECK( ret );

    // Run the local network scan codes
    scan_local_network();
}
