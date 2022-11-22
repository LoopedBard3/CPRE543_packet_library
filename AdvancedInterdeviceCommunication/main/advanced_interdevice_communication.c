/* Advanced Interdevice Communication Example

*/
#include <string.h>
#include <esp_wifi.h>
#include <esp_task_wdt.h>
#include <esp_log.h>
#include <esp_event.h>
#include <nvs_flash.h>
#include "packet_library.h"

#define MIDDLE_MONITOR false // Set true if the ESP-32 to be flashed should act as the middle-man

// Variables that need to be setup for this example to work
uint8_t station_target_macs[2][6] = {(uint8_t []){ 0x78, 0x21, 0x84, 0xB9, 0x38, 0x1C }, (uint8_t []){ 0x78, 0x21, 0x84, 0xB9, 0x05, 0xD8 }}; // MAC addresses of the ESP-32's that will act as the stations sending packets to each other
uint8_t middle_monitor_mac[6] = { 0x78, 0x21, 0x84, 0xB9, 0x36, 0x20 }; // MAC address of the ESP-32 that will act as the middle man
uint8_t pkt_payload[6] = { 0x50, 0x51, 0x52, 0x53, 0x54, 0x55 }; // Payload that the stations will send each other
int station_pkt_payload_length = 6; // Length of the payload the stations will send to each other

// Variables used in the code, no changes necessary to work
int counter = 0; // Counter for stations to keep track of the number of received packets to trigger sending a packet
uint8_t mac[6]; // Holder for this ESP-32's MAC address
uint8_t target_mac[6]; // Holder for the target MAC address
wifi_mac_data_frame_t* station_pkt; // Pointer to the allocated packet that the stations send out

// Create the callback for when we get a packet as the middle man device
static void receive_middle_man_general_callback(wifi_mac_data_frame_t* packet, int payload_length)
{
    ESP_LOGI(LOGGING_TAG, "Packet Received");
    // Check if the packet is targeting this device (the middle-man)
    if(packet->address_1[0] == mac[0] && packet->address_1[1] == mac[1] && packet->address_1[2] == mac[2] 
    && packet->address_1[3] == mac[3] && packet->address_1[4] == mac[4] && packet->address_1[5] == mac[5])
    {
        ESP_LOGI(LOGGING_TAG, "Packet To Forward Received"); // Log that we got a packet to forward to a station
        /*  Generate the packet to send to the station
            This includes:
                Setting ADDR_1(Receiving MAC) to the target MAC in ADDR_3
                Setting ADDR_2(Sending MAC) to our MAC address
                Setting ADDR_3(Originator MAC) to the MAC in ADDR_2 since we only do station to station forwarding
                Copying the payload into this new packet
        */ 
        wifi_mac_data_frame_t* pkt = alloc_packet_custom(
            0x0008, // bits are read in opposite order, this denotes a general data packet
            0xFA, 
            packet->address_3,
            mac,
            packet->address_2,
            0xFA,
            (uint8_t []){ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
            payload_length,
            packet->payload
        );

        log_packet_annotated(pkt, payload_length, LOGGING_TAG);// Log packet with annotated logging helper
        send_packet_simple(pkt, payload_length); // Send the packet
        ESP_LOGI(LOGGING_TAG, "Packet Sent");
        free(pkt); // Free the allocated packet space for the next packet
    }
}

// Create the callback for when we get a packet as one of the two stations
static void receive_station_general_callback(wifi_mac_data_frame_t* packet, int payload_length)
{
    ESP_LOGI(LOGGING_TAG, "Packet Received %d", counter);
    counter++;  // Increment each time a packet is received by the station
    // Send a packet for every 32 packet we receive. This provides continuous packets being sent back and forth between the stations
    if(counter % 32 == 0)
    {
        ESP_LOGI(LOGGING_TAG, "SENDING PACKET");
        send_packet_simple(station_pkt, station_pkt_payload_length); // Send the packet 'station_pkt' generated in 'advanced_interdevice_communication'. This allows use to only have to generate the packet once, reusing it.
        counter = 0;
        ESP_LOGI(LOGGING_TAG, "PACKET SENT");
    }

    // Log packets received from the middle-man ESP-32 if they are for use
    if(packet->address_2[0] == middle_monitor_mac[0] && packet->address_2[1] == middle_monitor_mac[1] && packet->address_2[2] == middle_monitor_mac[2] 
    && packet->address_2[3] == middle_monitor_mac[3] && packet->address_2[4] == middle_monitor_mac[4] && packet->address_2[5] == middle_monitor_mac[5]
    && packet->address_1[0] == mac[0] && packet->address_1[1] == mac[1] && packet->address_1[2] == mac[2] 
    && packet->address_1[3] == mac[3] && packet->address_1[4] == mac[4] && packet->address_1[5] == mac[5])
    {
        ESP_LOGI(LOGGING_TAG, "PACKET RECEIVED FROM MIDDLE_MONITOR");
        log_packet_annotated(packet, payload_length, LOGGING_TAG); // Log the packet with annotations
    }
}

/* Advanced Communication method */
static void advanced_interdevice_communication(void)
{
    // Setup the wifi configuration, set the ESP-32 to act as a station, and store+log the ESP-32's MAC 
    ESP_ERROR_CHECK(setup_wifi_station_simple());
    ESP_ERROR_CHECK(setup_sta_default());
    ESP_ERROR_CHECK(esp_wifi_get_mac(WIFI_IF_STA, mac));
    ESP_LOGI(LOGGING_TAG, "MAC %02X:%02X:%02X:%02X:%02X:%02X", mac[0],mac[1], mac[2], mac[3], mac[4], mac[5]);

    // Filter out non-DATA packets
    wifi_promiscuous_filter_t packet_filter = {
        .filter_mask = WIFI_PROMIS_FILTER_MASK_DATA
    };
    ESP_ERROR_CHECK(setup_packets_type_filter(&packet_filter));


    if(MIDDLE_MONITOR) 
    {
        // If we are the middle monitor, all we care about is running the middle-man general callback.
        ESP_ERROR_CHECK(setup_promiscuous_simple_with_general_callback(&receive_middle_man_general_callback));
    }
    else 
    {
        // Set the target_mac to the station mac that does not match our MAC address
        if(station_target_macs[0][0] == mac[0] && station_target_macs[0][1] == mac[1] && station_target_macs[0][2] == mac[2] 
        && station_target_macs[0][3] == mac[3] && station_target_macs[0][4] == mac[4] && station_target_macs[0][5] == mac[5])
        {
            target_mac[0] = station_target_macs[1][0];
            target_mac[1] = station_target_macs[1][1];
            target_mac[2] = station_target_macs[1][2];
            target_mac[3] = station_target_macs[1][3];
            target_mac[4] = station_target_macs[1][4];
            target_mac[5] = station_target_macs[1][5];
        }
        else 
        {
            target_mac[0] = station_target_macs[0][0];
            target_mac[1] = station_target_macs[0][1];
            target_mac[2] = station_target_macs[0][2];
            target_mac[3] = station_target_macs[0][3];
            target_mac[4] = station_target_macs[0][4];
            target_mac[5] = station_target_macs[0][5];
        }

        // Allocate the packet that we are going to be sending to the other station through the middle-man ESP-32
        station_pkt = alloc_packet_custom(
            0x0008, // bits are read in opposite order 
            0xFA, 
            middle_monitor_mac,
            mac,
            target_mac,
            0xFA,
            (uint8_t []){ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
            station_pkt_payload_length,
            pkt_payload
        );

        // Setup the promiscuous capabilities to receive packets and run the station callback
        ESP_ERROR_CHECK(setup_promiscuous_simple_with_general_callback(&receive_station_general_callback));
    }
}

void app_main(void)
{
    // Initialize NVS (Not part of packet component functionality)
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
