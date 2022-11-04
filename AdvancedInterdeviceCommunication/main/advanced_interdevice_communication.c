/* Advanced Interdevice Communication Example

*/
#include <string.h>
#include <esp_wifi.h>
#include <esp_task_wdt.h>
#include <esp_log.h>
#include <esp_event.h>
#include <nvs_flash.h>
#include "packet_library.h"

#define MIDDLE_MONITOR false

int counter = 0;
uint8_t mac[6];
uint8_t middle_monitor_mac[6] = { 0x50, 0x51, 0x52, 0x53, 0x54, 0x55 };
uint8_t pkt_payload[6] = { 0x50, 0x51, 0x52, 0x53, 0x54, 0x55 };
int station_pkt_payload_length = 6;
wifi_mac_data_frame_t* station_pkt;

// Create the callback we want to use when we get a packet
static void receive_middle_man_general_callback(wifi_mac_data_frame_t* packet, int payload_length)
{
    ESP_LOGI(LOGGING_TAG, "Packet Received");
    if(packet->address_2[0] == mac[0] && packet->address_2[1] == mac[1] && packet->address_2[2] == mac[2] 
    && packet->address_2[3] == mac[3] && packet->address_2[4] == mac[4] && packet->address_2[5] == mac[5])
    {
        ESP_LOGI(LOGGING_TAG, "Packet To Forward Received");
        wifi_mac_data_frame_t* pkt = alloc_packet_custom(
            0x0008, // bits are read in opposite order 
            0xFA, 
            (uint8_t []){ 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC },
            mac,
            packet->address_3,
            0xFA,
            (uint8_t []){ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
            payload_length,
            packet->payload
        );
        ESP_LOGI(LOGGING_TAG, "Packet To Send");
        // Log packets from the middle_monitor_mac and not from us
        log_packet_annotated(pkt, payload_length, LOGGING_TAG);
        send_packet_simple(pkt, payload_length);
        ESP_LOGI(LOGGING_TAG, "Packet Sent");
        free(pkt);
    }
}

// Create the callback we want to use when we get a packet
static void receive_station_general_callback(wifi_mac_data_frame_t* packet, int payload_length)
{
    ESP_LOGI(LOGGING_TAG, "Packet Received");

    counter++;
    if(counter % 16 == 0)
    {
        ESP_LOGI(LOGGING_TAG, "SENDING PACKET");
        send_packet_simple(station_pkt, station_pkt_payload_length);
        ESP_LOGI(LOGGING_TAG, "PACKET SENT");
    }

    if(packet->address_2[0] == middle_monitor_mac[0] && packet->address_2[1] == middle_monitor_mac[1] && packet->address_2[2] == middle_monitor_mac[2] 
    && packet->address_2[3] == middle_monitor_mac[3] && packet->address_2[4] == middle_monitor_mac[4] && packet->address_2[5] == middle_monitor_mac[5]
    && packet->address_3[0] != mac[0] && packet->address_3[1] != mac[1] && packet->address_3[2] != mac[2] 
    && packet->address_3[3] != mac[3] && packet->address_3[4] != mac[4] && packet->address_3[5] != mac[5])
    {
        // Log packets from the middle_monitor_mac and not from us
        log_packet_annotated(packet, payload_length, LOGGING_TAG);
    }
}

/* Advanced Communication method */
static void advanced_interdevice_communication(void)
{
    setup_wifi_simple();
    setup_sta_default();
    ESP_ERROR_CHECK(esp_wifi_get_mac(WIFI_IF_STA, mac));
    ESP_LOGI(LOGGING_TAG, "MAC %02X:%02X:%02X:%02X:%02X:%02X", mac[0],mac[1], mac[2], mac[3], mac[4], mac[5]);

    wifi_promiscuous_filter_t packet_filter = {
        .filter_mask = WIFI_PROMIS_FILTER_MASK_DATA
    };
    ESP_ERROR_CHECK(setup_packets_type_filter(&packet_filter));

    if(MIDDLE_MONITOR) 
    {
        // Setup sta and promisc capabilities
        ESP_ERROR_CHECK(set_receive_callback_general(&receive_middle_man_general_callback));
        ESP_ERROR_CHECK(setup_sta_and_promiscuous_simple());
    }
    else
    {
        station_pkt = alloc_packet_custom(
            0x0008, // bits are read in opposite order 
            0xFA, 
            middle_monitor_mac,
            mac,
            (uint8_t []){ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
            0xFA,
            (uint8_t []){ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
            station_pkt_payload_length,
            pkt_payload
        );

        // Setup sta and promisc capabilities
        ESP_ERROR_CHECK(set_receive_callback_general(&receive_station_general_callback));
        ESP_ERROR_CHECK(setup_sta_and_promiscuous_simple());
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
    // Make sure nvs got setup properly
    ESP_ERROR_CHECK( ret );

    // Run the advanced communication
    advanced_interdevice_communication();
}
