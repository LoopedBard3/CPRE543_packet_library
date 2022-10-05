#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "packet_library.h"

// Private helper static types
static callback_setup_t promisc_callback_setup;
static callback_setup_t send_callback_setup;
static configuration_settings_t configuration_holder;

// Private Helper functions
static void promisc_simple_callback(void *buf, wifi_promiscuous_pkt_type_t type)
{
    ESP_LOGI(LOGGING_TAG, "START SIMPLE POMISC CALLBACK");
    wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
    //int length = pkt->rx_ctrl.sig_len; // TODO: Use for the payload callback
    wifi_mac_data_frame_t *frame = (wifi_mac_data_frame_t *)pkt->payload;

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
}

// Setup/configuration methods
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

// Send and receive full control
esp_err_t send_packet_raw(const void* buffer, int length, bool en_sys_seq)
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
    ESP_LOGI(LOGGING_TAG, "START SIMPLE SEND PACKET");
    if(configuration_holder.wifi_interface_set != true)
    {
        return ESP_ERR_WIFI_IF;
    }
    int length = sizeof(packet) - sizeof(uint8_t *) + payload_length;
    // Do the simple callback and then each individual callback action
    // TODO: Update these for send
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
    esp_wifi_80211_tx(configuration_holder.wifi_interface, (void *)packet, length, false);
    return ESP_OK;
}

// **************************************************
// Receive callback methods
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


// **************************************************
// Send callback methods
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


