#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "packet_library.h"
#include <stdio.h>
#include <string.h>

// Private helper static types
static callback_setup_t promisc_callback_setup;
static callback_setup_t send_callback_setup;
static configuration_settings_t configuration_holder;

// Private Helper functions
static void print_packet_type_testing(wifi_promiscuous_pkt_type_t type)
{
    if(type == WIFI_PKT_MGMT)
    {
        ESP_LOGI(LOGGING_TAG, "Management packet");
    }
    else if(type == WIFI_PKT_CTRL)
    { 
        ESP_LOGI(LOGGING_TAG, "Control packet");        
    }
    else if(type == WIFI_PKT_DATA)
    { 
        ESP_LOGI(LOGGING_TAG, "Data packet");        
    }
    else if(type == WIFI_PKT_MISC)
    { 
        ESP_LOGI(LOGGING_TAG, "Miscellaneous packet");        
    }
    else
    { 
        ESP_LOGI(LOGGING_TAG, "Unknown packet type");        
    }
} 

static void promisc_simple_callback(void *buf, wifi_promiscuous_pkt_type_t type)
{
    ESP_LOGI(LOGGING_TAG, "START SIMPLE PROMISC CALLBACK");
    const wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
    int payload_length = pkt->rx_ctrl.sig_len - sizeof(wifi_promiscuous_pkt_t);
    wifi_mac_data_frame_t *frame = (wifi_mac_data_frame_t *)pkt->payload;

    //print_packet_type_testing(type);
    if(promisc_callback_setup.precallback_print != DISABLE)
    {
        ESP_LOGI(LOGGING_TAG, "PROMISC PRE-CALLBACK PRINT START");
        if(promisc_callback_setup.precallback_print == ANNOTATED)
        {
            log_packet_annotated(frame, payload_length, LOGGING_TAG);
        } 
        else if(promisc_callback_setup.precallback_print == HEX)
        {
            log_packet_hex(frame, payload_length, LOGGING_TAG);
        }
        ESP_LOGI(LOGGING_TAG, "PROMISC PRE-CALLBACK PRINT END");
    }

    // Do the simple callback and then each individual callback action
    if (promisc_callback_setup.general_callback_is_set)
    {
        promisc_callback_setup.general_callback(frame);
    }
    if (promisc_callback_setup.frame_control_callback_is_set)
    {
        promisc_callback_setup.frame_control_callback(&frame->frame_control);
    }
    if (promisc_callback_setup.duration_id_callback_is_set)
    {
        promisc_callback_setup.duration_id_callback(&frame->duration_id);
    }
    if (promisc_callback_setup.address_1_callback_is_set)
    {
        promisc_callback_setup.address_1_callback(frame->address_1);
    }
    if (promisc_callback_setup.address_2_callback_is_set)
    {
        promisc_callback_setup.address_2_callback(frame->address_2);
    }
    if (promisc_callback_setup.address_3_callback_is_set)
    {
        promisc_callback_setup.address_3_callback(frame->address_3);
    }
    if (promisc_callback_setup.sequence_control_callback_is_set)
    {
        promisc_callback_setup.sequence_control_callback(&frame->sequence_control);
    }
    if (promisc_callback_setup.address_4_callback_is_set)
    {
        promisc_callback_setup.address_4_callback(frame->address_4);
    }
    if (promisc_callback_setup.payload_callback_is_set)
    {
        promisc_callback_setup.payload_callback(frame->payload, payload_length);
    }

    if(promisc_callback_setup.postcallback_print != DISABLE)
    {
        ESP_LOGI(LOGGING_TAG, "PROMISC POST-CALLBACK PRINT START");
        if(promisc_callback_setup.postcallback_print == ANNOTATED)
        {
            log_packet_annotated(frame, payload_length, LOGGING_TAG);
        } 
        else if(promisc_callback_setup.postcallback_print == HEX)
        {
            log_packet_hex(frame, payload_length, LOGGING_TAG);
        }
        ESP_LOGI(LOGGING_TAG, "PROMISC POST-CALLBACK PRINT END");
    }
}

// **************************************************
// Setup/configuration methods
// **************************************************
esp_err_t setup_wifi_simple()
{
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    return setup_wifi_custom(cfg);
}

esp_err_t setup_wifi_custom(wifi_init_config_t config)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);
    ESP_ERROR_CHECK(esp_wifi_init(&config));
    ESP_LOGI(LOGGING_TAG, "WIFI INITIALIZED");
    return ESP_OK;
}

esp_err_t setup_sta_default()
{
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    configuration_holder.wifi_interface = WIFI_IF_STA;
    configuration_holder.wifi_interface_set = true;
    return ESP_OK;
}

esp_err_t setup_packets_type_filter(const wifi_promiscuous_filter_t *type_filter)
{ 
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_filter(type_filter));
    return ESP_OK;
}

esp_err_t setup_promiscuous_default(wifi_promiscuous_cb_t callback)
{
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(callback));
    ESP_LOGI(LOGGING_TAG, "DEFAULT PROMISCUOUS CALLBACK SET");
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
    ESP_LOGI(LOGGING_TAG, "DEFAULT PROMISCUOUS STARTED");
    return ESP_OK;
}

esp_err_t setup_promiscuous_simple()
{
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(&promisc_simple_callback));
    ESP_LOGI(LOGGING_TAG, "SIMPLE PROMISCUOUS CALLBACK SET");
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
    ESP_LOGI(LOGGING_TAG, "SIMPLE PROMISCUOUS STARTED");
    return ESP_OK;
}

esp_err_t setup_promiscuous_simple_with_general_callback(packet_library_simple_callback_t simple_callback)
{
    promisc_callback_setup.general_callback = simple_callback;
    promisc_callback_setup.general_callback_is_set = true;
    return setup_promiscuous_simple();
}

esp_err_t remove_promiscuous_general_callback()
{
    promisc_callback_setup.general_callback_is_set = false;
    return ESP_OK;
}

// **************************************************
// Send Packet Methods
// **************************************************
esp_err_t send_packet_raw_no_callback(const void* buffer, int length, bool en_sys_seq)
{
    if(configuration_holder.wifi_interface_set != true)
    {
        return ESP_ERR_WIFI_IF;
    }
    esp_wifi_80211_tx(configuration_holder.wifi_interface, buffer, length, en_sys_seq);
    return ESP_OK;
}

esp_err_t send_packet_simple(wifi_mac_data_frame_t* packet, int payload_length) 
{
    int length = sizeof(wifi_mac_data_frame_t) + payload_length;
    ESP_LOGI(LOGGING_TAG, "START SIMPLE SEND PACKET: LENGTH %d", length);
    if(configuration_holder.wifi_interface_set != true)
    {
        return ESP_ERR_WIFI_IF;
    }

    if(send_callback_setup.precallback_print != DISABLE)
    {
        ESP_LOGI(LOGGING_TAG, "SEND PRE-CALLBACK PRINT START");
        if(send_callback_setup.precallback_print == ANNOTATED)
        {
            log_packet_annotated(packet, payload_length, LOGGING_TAG);
        } 
        else if(send_callback_setup.precallback_print == HEX)
        {
            log_packet_hex(packet, payload_length, LOGGING_TAG);
        }
        ESP_LOGI(LOGGING_TAG, "SEND PRE-CALLBACK PRINT END");
    }

    // Do the simple callback and then each individual callback action
    if (send_callback_setup.general_callback_is_set)
    {
        send_callback_setup.general_callback(packet);
    }
    if (send_callback_setup.frame_control_callback_is_set)
    {
        send_callback_setup.frame_control_callback(&packet->frame_control);
    }
    if (send_callback_setup.duration_id_callback_is_set)
    {
        send_callback_setup.duration_id_callback(&packet->duration_id);
    }
    if (send_callback_setup.address_1_callback_is_set)
    {
        send_callback_setup.address_1_callback(packet->address_1);
    }
    if (send_callback_setup.address_2_callback_is_set)
    {
        send_callback_setup.address_2_callback(packet->address_2);
    }
    if (send_callback_setup.address_3_callback_is_set)
    {
        send_callback_setup.address_3_callback(packet->address_3);
    }
    if (send_callback_setup.sequence_control_callback_is_set)
    {
        send_callback_setup.sequence_control_callback(&packet->sequence_control);
    }
    if (send_callback_setup.address_4_callback_is_set)
    {
        send_callback_setup.address_4_callback(packet->address_4);
    }
    if (send_callback_setup.payload_callback_is_set && &packet->payload)
    {
        send_callback_setup.payload_callback(packet->payload, payload_length);
    }

    if(send_callback_setup.postcallback_print != DISABLE)
    {
        ESP_LOGI(LOGGING_TAG, "SEND POST-CALLBACK PRINT START");
        if(send_callback_setup.postcallback_print == ANNOTATED)
        {
            log_packet_annotated(packet, payload_length, LOGGING_TAG);
        } 
        else if(send_callback_setup.postcallback_print == HEX)
        {
            log_packet_hex(packet, payload_length, LOGGING_TAG);
        }
        ESP_LOGI(LOGGING_TAG, "SEND POST-CALLBACK PRINT END");
    }

    esp_wifi_80211_tx(configuration_holder.wifi_interface, (void *)packet, length, false);
    return ESP_OK;
}

// **************************************************
// Receive Callback Methods
// **************************************************
esp_err_t set_receive_callback_frame_control(packet_library_frame_control_callback_t simple_callback)
{
    promisc_callback_setup.frame_control_callback = simple_callback;
    promisc_callback_setup.frame_control_callback_is_set = true;
    return ESP_OK;
}

esp_err_t set_receive_callback_duration_id(packet_library_duration_id_callback_t simple_callback)
{
    promisc_callback_setup.duration_id_callback = simple_callback;
    promisc_callback_setup.duration_id_callback_is_set = true;
    return ESP_OK;
}

esp_err_t set_receive_callback_address_1(packet_library_address_1_callback_t simple_callback)
{
    promisc_callback_setup.address_1_callback = simple_callback;
    promisc_callback_setup.address_1_callback_is_set = true;
    return ESP_OK;
}

esp_err_t set_receive_callback_address_2(packet_library_address_2_callback_t simple_callback)
{
    promisc_callback_setup.address_2_callback = simple_callback;
    promisc_callback_setup.address_2_callback_is_set = true;
    return ESP_OK;
}

esp_err_t set_receive_callback_address_3(packet_library_address_3_callback_t simple_callback)
{
    promisc_callback_setup.address_3_callback = simple_callback;
    promisc_callback_setup.address_3_callback_is_set = true;
    return ESP_OK;
}

esp_err_t set_receive_callback_address_4(packet_library_address_4_callback_t simple_callback)
{
    promisc_callback_setup.address_4_callback = simple_callback;
    promisc_callback_setup.address_4_callback_is_set = true;
    return ESP_OK;
}

esp_err_t set_receive_callback_sequence_control(packet_library_sequence_control_callback_t simple_callback)
{
    promisc_callback_setup.sequence_control_callback = simple_callback;
    promisc_callback_setup.sequence_control_callback_is_set = true;
    return ESP_OK;
}

esp_err_t set_receive_callback_payload(packet_library_payload_callback_t simple_callback){
    promisc_callback_setup.payload_callback = simple_callback;
    promisc_callback_setup.payload_callback_is_set = true;
    return ESP_OK;
}

esp_err_t set_receive_pre_callback_print(enum callback_print_option option)
{
    promisc_callback_setup.precallback_print = option;
    return ESP_OK;
}

esp_err_t set_receive_post_callback_print(enum callback_print_option option)
{
    promisc_callback_setup.postcallback_print = option;
    return ESP_OK;
}

esp_err_t remove_receive_callback_frame_control()
{
    promisc_callback_setup.frame_control_callback_is_set = false;
    return ESP_OK;
}

esp_err_t remove_receive_callback_duration_id()
{
    promisc_callback_setup.duration_id_callback_is_set = false;
    return ESP_OK;
}

esp_err_t remove_receive_callback_address_1()
{
    promisc_callback_setup.address_1_callback_is_set = false;
    return ESP_OK;
}

esp_err_t remove_receive_callback_address_2()
{
    promisc_callback_setup.address_2_callback_is_set = false;
    return ESP_OK;
}

esp_err_t remove_receive_callback_address_3()
{
    promisc_callback_setup.address_3_callback_is_set = false;
    return ESP_OK;
}

esp_err_t remove_receive_callback_address_4()
{
    promisc_callback_setup.address_4_callback_is_set = false;
    return ESP_OK;
}

esp_err_t remove_receive_callback_sequence_control()
{
    promisc_callback_setup.sequence_control_callback_is_set = false;
    return ESP_OK;
}

esp_err_t remove_receive_callback_payload()
{
    promisc_callback_setup.payload_callback_is_set = false;
    return ESP_OK;
}

// **************************************************
// Send Callback Methods
// **************************************************

esp_err_t set_send_callback_frame_control(packet_library_frame_control_callback_t simple_callback)
{
    send_callback_setup.frame_control_callback = simple_callback;
    send_callback_setup.frame_control_callback_is_set = true;
    return ESP_OK;
}

esp_err_t set_send_callback_duration_id(packet_library_duration_id_callback_t simple_callback)
{
    send_callback_setup.duration_id_callback = simple_callback;
    send_callback_setup.duration_id_callback_is_set = true;
    return ESP_OK;
}

esp_err_t set_send_callback_address_1(packet_library_address_1_callback_t simple_callback)
{
    send_callback_setup.address_1_callback = simple_callback;
    send_callback_setup.address_1_callback_is_set = true;
    return ESP_OK;
}

esp_err_t set_send_callback_address_2(packet_library_address_2_callback_t simple_callback)
{
    send_callback_setup.address_2_callback = simple_callback;
    send_callback_setup.address_2_callback_is_set = true;
    return ESP_OK;
}

esp_err_t set_send_callback_address_3(packet_library_address_3_callback_t simple_callback)
{
    send_callback_setup.address_3_callback = simple_callback;
    send_callback_setup.address_3_callback_is_set = true;
    return ESP_OK;
}

esp_err_t set_send_callback_address_4(packet_library_address_4_callback_t simple_callback)
{
    send_callback_setup.address_4_callback = simple_callback;
    send_callback_setup.address_4_callback_is_set = true;
    return ESP_OK;
}

esp_err_t set_send_callback_sequence_control(packet_library_sequence_control_callback_t simple_callback)
{
    send_callback_setup.sequence_control_callback = simple_callback;
    send_callback_setup.sequence_control_callback_is_set = true;
    return ESP_OK;
}

esp_err_t set_send_callback_payload(packet_library_payload_callback_t simple_callback)
{
    send_callback_setup.payload_callback = simple_callback;
    send_callback_setup.payload_callback_is_set = true;
    return ESP_OK;
}

esp_err_t set_send_pre_callback_print(enum callback_print_option option)
{
    send_callback_setup.precallback_print = option;
    return ESP_OK;
}

esp_err_t set_send_post_callback_print(enum callback_print_option option)
{
    send_callback_setup.postcallback_print = option;
    return ESP_OK;
}

esp_err_t remove_send_callback_frame_control()
{
    send_callback_setup.frame_control_callback_is_set = false;
    return ESP_OK;
}

esp_err_t remove_send_callback_duration_id()
{
    send_callback_setup.duration_id_callback_is_set = false;
    return ESP_OK;
}

esp_err_t remove_send_callback_address_1()
{
    send_callback_setup.address_1_callback_is_set = false;
    return ESP_OK;
}

esp_err_t remove_send_callback_address_2()
{
    send_callback_setup.address_2_callback_is_set = false;
    return ESP_OK;
}

esp_err_t remove_send_callback_address_3()
{
    send_callback_setup.address_3_callback_is_set = false;
    return ESP_OK;
}

esp_err_t remove_send_callback_address_4()
{
    send_callback_setup.address_4_callback_is_set = false;
    return ESP_OK;
}

esp_err_t remove_send_callback_sequence_control()
{
    send_callback_setup.sequence_control_callback_is_set = false;
    return ESP_OK;
}

esp_err_t remove_send_callback_payload()
{
    send_callback_setup.payload_callback_is_set = false;
    return ESP_OK;
}

// **************************************************
// General Helper Methods
// **************************************************
esp_err_t log_packet_annotated(wifi_mac_data_frame_t* packet, int payload_length, const char * TAG)
{
    ESP_LOGI(TAG, "Frame Control: %04X\nDuration_ID: %04X\nPacket Addr1: %02X:%02X:%02X:%02X:%02X:%02X\nAddr2: %02X:%02X:%02X:%02X:%02X:%02X\nAddr3 %02X:%02X:%02X:%02X:%02X:%02X\nSequence Control: %04X\nAddr4 %02X:%02X:%02X:%02X:%02X:%02X",
        packet->frame_control, packet->duration_id,
        packet->address_1[0],packet->address_1[1], packet->address_1[2], packet->address_1[3], packet->address_1[4], packet->address_1[5],
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

esp_err_t log_packet_hex(wifi_mac_data_frame_t* packet, int payload_length, const char * TAG)
{
    ESP_LOGI(TAG, "%04X%04X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%04X%02X%02X%02X%02X%02X%02X",
        packet->frame_control, packet->duration_id,
        packet->address_1[0],packet->address_1[1], packet->address_1[2], packet->address_1[3], packet->address_1[4], packet->address_1[5],
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
            snprintf(temp_buffer, 3, "%02X", packet->payload[counter]);
            memcpy(&payload_buffer[counter*2], temp_buffer, 2);
        }

        ESP_LOGI(TAG, "%.*s", payload_length*2, payload_buffer);
        free(payload_buffer);
    }
    return ESP_OK;    
}

wifi_mac_data_frame_t* alloc_packet_custom(uint16_t frame_control, uint16_t duration_id, uint8_t address_1[6], uint8_t address_2[6], uint8_t address_3[6], uint16_t sequence_control, uint8_t address_4[6], int payload_length, uint8_t* payload)
{
    wifi_mac_data_frame_t* pkt = calloc(1, sizeof(wifi_mac_data_frame_t) + payload_length);
    pkt->frame_control = frame_control;
    pkt->duration_id = duration_id;
    memcpy(pkt->address_1, address_1, sizeof(uint8_t[6]));
    memcpy(pkt->address_2, address_2, sizeof(uint8_t[6]));
    memcpy(pkt->address_3, address_3, sizeof(uint8_t[6]));
    pkt->sequence_control = sequence_control;
    memcpy(pkt->address_4, address_4, sizeof(uint8_t[6]));
    if(payload_length > 0){
        memcpy(&pkt->payload, payload, payload_length);
    }
    return pkt;
}

wifi_mac_data_frame_t* alloc_packet_default_payload(int payload_length, uint8_t *payload)
{
    return alloc_packet_custom(0, 0, (uint8_t []){0,0,0,0,0,0}, (uint8_t []){0,0,0,0,0,0}, (uint8_t []){0,0,0,0,0,0}, 0, (uint8_t []){0,0,0,0,0,0}, payload_length, payload);
}

wifi_mac_data_frame_t* alloc_packet_default(int payload_length)
{
    return alloc_packet_default_payload(payload_length, NULL);
}
