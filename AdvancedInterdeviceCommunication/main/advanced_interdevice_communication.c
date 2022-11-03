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


/* Advanced Communication method */
static void advanced_interdevice_communication(void)
{
    setup_wifi_simple();
    //ESP_ERROR_CHECK(set_receive_pre_callback_print(DENOTE));
    //ESP_ERROR_CHECK(set_receive_post_callback_print(ANNOTATED));
    ESP_ERROR_CHECK(set_send_pre_callback_print(DENOTE));
    //ESP_ERROR_CHECK(set_send_post_callback_print(ANNOTATED));

    if(MIDDLE_MONITOR) 
    {
        wifi_promiscuous_filter_t packet_filter = {
            .filter_mask = WIFI_PROMIS_FILTER_MASK_DATA
        };
        ESP_ERROR_CHECK(setup_packets_type_filter(&packet_filter));

        //ESP_ERROR_CHECK(setup_promiscuous_simple_with_general_callback(&double_general_callback));

    }
    else
    {
        uint8_t mac[6];
        setup_sta_default();
        ESP_ERROR_CHECK(esp_wifi_get_mac(WIFI_IF_STA, mac));
        ESP_LOGI(LOGGING_TAG, "MAC %02X:%02X:%02X:%02X:%02X:%02X", mac[0],mac[1], mac[2], mac[3], mac[4], mac[5]);
        
        uint8_t payload[6] = { 0x50, 0x51, 0x52, 0x53, 0x54, 0x55 };
        int payload_length = 6;
        wifi_mac_data_frame_t* pkt = alloc_packet_custom(
            0x0008, //  bits are read in opposite order 
            0xFA, 
            (uint8_t []){ 0x10, 0x11, 0x12, 0x13, 0x14, 0x15 },
            (uint8_t []){ 0x20, 0x21, 0x22, 0x23, 0x24, 0x25 },
            (uint8_t []){ 0x30, 0x31, 0x32, 0x33, 0x34, 0x35 },
            0xFA,
            (uint8_t []){ 0x40, 0x41, 0x42, 0x43, 0x44, 0x45 },
            payload_length,
            payload
        );

        // Setup sta and promisc capabilities
        ESP_ERROR_CHECK(setup_sta_and_promiscuous_simple());
        //ESP_ERROR_CHECK(set_receive_callback_general(&double_general_callback));
        

        const TickType_t xDelay = 1000 / portTICK_PERIOD_MS; 
        while(true){
            ESP_LOGI(LOGGING_TAG, "Entering send mode; SENDING PACKET");
            switch_between_sta_and_promis(true);
            send_packet_simple(pkt, payload_length);
            ESP_LOGI(LOGGING_TAG, "PACKET SENT; Entering Promisc Mode");
            switch_between_sta_and_promis(false);
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

    // Run the advanced communication
    advanced_interdevice_communication();
}
