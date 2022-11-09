/* Basic Callback Example


*/

#include <string.h>
#include <esp_wifi.h>
#include <esp_task_wdt.h>
#include <esp_log.h>
#include <esp_event.h>
#include <nvs_flash.h>
#include "packet_library.h"

#define SENDER false // Sets whether this device should send packets
#define RECEIVER !SENDER // Sets if the device is the receiver to the opposite of sender
#define DO_GENERAL_CALLBACK true // Enables General Callback Example
#define DO_INDIVIDUAL_CALLBACK true // Enables Individual Field Callback Examples

// Simple general packet callback that has access to the whole packet, doubles duration id in this case. (Used when sending and receiving)
static void double_general_callback(wifi_mac_data_frame_t* packet)
{
    packet->duration_id = packet->duration_id * 2;
    ESP_LOGI(LOGGING_TAG, "General Callback, New DurationID: %04X", packet->duration_id);
}

// Simple individual field callback that doubles the packets duration id (Used when receiving)
static void double_duration_id_callback(uint16_t* duration_id)
{
    *duration_id = *duration_id * 2;    
    ESP_LOGI(LOGGING_TAG, "Individual Callback, New DurationID: %04X", *duration_id);
}

// Simple individual field callback that sets the packets duration id based on current value, causing it to flip-flop (Used when sending)
static void set_duration_id_callback(uint16_t* duration_id)
{
    if(*duration_id == 0xBEEE)
    {
        *duration_id = 0xCAB0;
    } 
    else
    {
        *duration_id = 0xBEEE;
    }   
    ESP_LOGI(LOGGING_TAG, "Individual Callback, New DurationID: %04X", *duration_id);
}

// Simple individual field callback that sets the second value in address 1 field to twice the value in the first value (Used when sending and receiving)
static void double_address_1_callback(uint8_t address_1[6])
{
    address_1[1] = address_1[0] * 2;    
    ESP_LOGI(LOGGING_TAG, "Individual Callback, New Address 1[1]: %02X", address_1[1]);
}

// Simple individual field callback that sets the third payload value to 255/0xFF if there is enough data (Used when sending and receiving)
static void payload_callback(uint8_t payload[], int payload_length)
{
    if(payload_length > 3)
    {
        payload[3] = 255;
    }
    ESP_LOGI(LOGGING_TAG, "Individual Callback, Payload Length: %d", payload_length);
}

/* Main method used for setting up the callbacks */
static void basic_callbacks(void)
{
    // Setup with default WiFi setup
    setup_wifi_station_simple();

    if(RECEIVER)
    {
        // Enable the printing of received packets with annotations before and after all callbacks are run.
        ESP_ERROR_CHECK(set_receive_pre_callback_print(ANNOTATED));
        ESP_ERROR_CHECK(set_receive_post_callback_print(ANNOTATED));
        
        // Setup a packet filter so that the only packets we run callbacks on are data packets
        wifi_promiscuous_filter_t packet_filter = {
            .filter_mask = WIFI_PROMIS_FILTER_MASK_DATA
        };
        ESP_ERROR_CHECK(setup_packets_type_filter(&packet_filter));

        // Setup the individual callbacks is they are enabled, otherwise skip setting them up
        if(DO_INDIVIDUAL_CALLBACK)
        {
            ESP_ERROR_CHECK(set_receive_callback_duration_id(&double_duration_id_callback));
            ESP_ERROR_CHECK(set_receive_callback_address_1(&double_address_1_callback));
            ESP_ERROR_CHECK(set_receive_callback_payload(&payload_callback)); 
        }

        // Setup the general callback if it is enabled, otherwise just enable promiscuous mode
        if(DO_GENERAL_CALLBACK)
        {
            ESP_ERROR_CHECK(setup_promiscuous_simple_with_general_callback(&double_general_callback));
        }
        else
        {
            ESP_ERROR_CHECK(setup_promiscuous_simple());
        }
    }

    if(SENDER)
    {
        // Setup the device as a station so it can send packets
        setup_sta_default();

        // Enable the printing of received packets with annotations before and after all callbacks are run.
        ESP_ERROR_CHECK(set_send_pre_callback_print(ANNOTATED));
        ESP_ERROR_CHECK(set_send_post_callback_print(ANNOTATED));

        // Create a payload to include in out custom send packet
        uint8_t payload[6] = { 0x50, 0x51, 0x52, 0x53, 0x54, 0x55 };
        int payload_length = 6;

        // Generate the packet we want to send with easy to spot values
        wifi_mac_data_frame_t* pkt = alloc_packet_custom(
            0x0008, //  bits are read in opposite order, this value will set packet as a data type
            0xFA, 
            (uint8_t []){ 0x10, 0x11, 0x12, 0x13, 0x14, 0x15 },
            (uint8_t []){ 0x20, 0x21, 0x22, 0x23, 0x24, 0x25 },
            (uint8_t []){ 0x30, 0x31, 0x32, 0x33, 0x34, 0x35 },
            0xFA,
            (uint8_t []){ 0x40, 0x41, 0x42, 0x43, 0x44, 0x45 },
            payload_length,
            payload
        );

        // Setup the individual callbacks is they are enabled, otherwise skip setting them up
        if(DO_INDIVIDUAL_CALLBACK)
        {
            ESP_ERROR_CHECK(set_send_callback_duration_id(&set_duration_id_callback));
            ESP_ERROR_CHECK(set_send_callback_address_1(&double_address_1_callback));
            ESP_ERROR_CHECK(set_send_callback_payload(&payload_callback)); // Payload includes FCS which is not included in the send setup causing a size disparity between what was sent and received
        }

        // Setup the general callback if it is enabled
        if(DO_GENERAL_CALLBACK)
        {
            ESP_ERROR_CHECK(set_send_callback_general(&double_general_callback));
        }

        // Setup the packet to be repeatadly send every 200ms with Packet Sent as a message
        const TickType_t xDelay = 200 / portTICK_PERIOD_MS; 
        while(true){
            send_packet_simple(pkt, payload_length);
            ESP_LOGI(LOGGING_TAG, "PACKET SENT");
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
    // Make sure nvs got setup properly
    ESP_ERROR_CHECK( ret );

    // Run the callback example method
    basic_callbacks();
}
