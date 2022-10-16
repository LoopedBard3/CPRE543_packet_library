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
#include <esp_wifi.h>
#include <esp_task_wdt.h>
#include <esp_log.h>
#include <esp_event.h>
#include "nvs_flash.h"
#include "packet_library.h"

#define SENDER true
#define RECEIVER !SENDER
#define DO_GENERAL_CALLBACK_TEST true
#define DO_INDIVIDUAL_CALLBACK_TEST true

static void double_general_callback(wifi_mac_data_frame_t* packet)
{
    packet->duration_id = packet->duration_id * 2;
    ESP_LOGI(LOGGING_TAG, "CB: G, O DI: %04X", packet->duration_id);
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
static void set_sequence_control_callback(uint16_t* sequence_control)
{
    if(*sequence_control == 0xBEEE)
    {
        *sequence_control = 0xCAB0;
    } 
    else
    {
        *sequence_control = 0xBEEE;
    }
        
    ESP_LOGI(LOGGING_TAG, "CB: SC, NV: %04X", *sequence_control);
}
static void double_address_4_callback(uint8_t address_4[6])
{
    address_4[1] = address_4[0] * 2;    
    ESP_LOGI(LOGGING_TAG, "CB: A4, NV: %02X", address_4[1]);
}
static void payload_callback(uint8_t payload[], int payload_length)
{
    if(payload_length > 3)
    {
        payload[3] = 255;
    }
    ESP_LOGI(LOGGING_TAG, "CB: P; L: %d", payload_length);
}

/* Library test method */
static void library_test(void)
{
    setup_wifi_simple();

    if(RECEIVER) // TODO: Debug possible memory leak?
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
            ESP_ERROR_CHECK(set_receive_callback_payload(&payload_callback)); // Payload includes FCS which is not included in the send setup (explains size disparity)
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
        uint8_t mac[6];
        setup_sta_default();
        ESP_ERROR_CHECK(set_send_pre_callback_print(ANNOTATED));
        ESP_ERROR_CHECK(set_send_post_callback_print(ANNOTATED));
        ESP_ERROR_CHECK(esp_wifi_get_mac(WIFI_IF_STA, mac));
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


        if(DO_INDIVIDUAL_CALLBACK_TEST)
        {
            //ESP_ERROR_CHECK(set_send_callback_frame_control(&double_frame_control_callback));
            ESP_ERROR_CHECK(set_send_callback_duration_id(&set_duration_id_callback));
            ESP_ERROR_CHECK(set_send_callback_address_1(&double_address_1_callback));
            ESP_ERROR_CHECK(set_send_callback_address_2(&double_address_2_callback));
            ESP_ERROR_CHECK(set_send_callback_address_3(&double_address_3_callback));
            ESP_ERROR_CHECK(set_send_callback_address_4(&double_address_4_callback));
            ESP_ERROR_CHECK(set_send_callback_sequence_control(&set_sequence_control_callback));
            ESP_ERROR_CHECK(set_send_callback_payload(&payload_callback)); // Payload includes FCS which is not included in the send setup (explains size disparity)
        }

        if(DO_GENERAL_CALLBACK_TEST)
        {
            ESP_ERROR_CHECK(set_send_callback_general(&double_general_callback));
        }

        const TickType_t xDelay = 10 / portTICK_PERIOD_MS; 
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
    ESP_ERROR_CHECK( ret );

    //esp_log_level_set(LOGGING_TAG, ESP_LOG_ERROR);
    library_test();
}
