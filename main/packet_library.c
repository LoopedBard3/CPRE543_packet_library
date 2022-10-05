#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "packet_library.h"

// Private helper static types
static promisc_callback_setup_t callback_setup;

// Private Helper functions
static void promisc_simple_callback(void *buf, wifi_promiscuous_pkt_type_t type)
{
    ESP_LOGI(LOGGING_TAG, "START SIMPLE POMISC CALLBACK");
    wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
    int length = pkt->rx_ctrl.sig_len;
    wifi_mac_data_frame_t *frame = (wifi_mac_data_frame_t *)pkt->payload;
    // Do the simple callback and then each individual callback action
    if (callback_setup.general_callback_is_set)
    {
        callback_setup.general_callback(frame);
    }
    if (callback_setup.frame_control_callback_is_set)
    {
        callback_setup.frame_control_callback(&frame->frame_control);
    }
    if (callback_setup.duration_id_callback_is_set)
    {
        callback_setup.duration_id_callback(&frame->duration_id);
    }
    if (callback_setup.address_1_callback_is_set)
    {
        callback_setup.address_1_callback(frame->address_1);
    }
    if (callback_setup.address_2_callback_is_set)
    {
        callback_setup.address_2_callback(frame->address_2);
    }
    if (callback_setup.address_3_callback_is_set)
    {
        callback_setup.address_3_callback(frame->address_3);
    }
    if (callback_setup.sequence_control_callback_is_set)
    {
        callback_setup.sequence_control_callback(&frame->sequence_control);
    }
    if (callback_setup.address_4_callback_is_set)
    {
        callback_setup.address_4_callback(frame->address_4);
    }
}

// Setup/configuration methods
esp_err_t setup_wifi_simple()
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_LOGI(LOGGING_TAG, "SIMPLE WIFI INITIALIZED");
    return ESP_OK;
}

esp_err_t setup_wifi_custom() // TODO
{
    return ESP_OK;
}

esp_err_t setup_sta_default()
{
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
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
    callback_setup.general_callback = simple_callback;
    callback_setup.general_callback_is_set = true;
    ESP_ERROR_CHECK(setup_promiscuous_simple());
    return ESP_OK;
}

// Send and receive full control
esp_err_t send_packet_raw() // TODO: Call the send callback functions then send
{
    return ESP_OK;
}

esp_err_t send_packet_simple() // TODO: Call the send callback functions then send
{
    return ESP_OK;
}

// Individual field callbacks/send options
esp_err_t set_callback_frame_control(packet_library_frame_control_callback_t simple_callback)
{
    callback_setup.frame_control_callback = simple_callback;
    callback_setup.frame_control_callback_is_set = true;
    return ESP_OK;
}

esp_err_t set_callback_duration_id(packet_library_duration_id_callback_t simple_callback)
{
    callback_setup.duration_id_callback = simple_callback;
    callback_setup.duration_id_callback_is_set = true;
    return ESP_OK;
}

esp_err_t set_callback_address_1(packet_library_address_1_callback_t simple_callback)
{
    callback_setup.address_1_callback = simple_callback;
    callback_setup.address_1_callback_is_set = true;
    return ESP_OK;
}

esp_err_t set_callback_address_2(packet_library_address_2_callback_t simple_callback)
{
    callback_setup.address_2_callback = simple_callback;
    callback_setup.address_2_callback_is_set = true;
    return ESP_OK;
}

esp_err_t set_callback_address_3(packet_library_address_3_callback_t simple_callback)
{
    callback_setup.address_3_callback = simple_callback;
    callback_setup.address_3_callback_is_set = true;
    return ESP_OK;
}

esp_err_t set_callback_address_4(packet_library_address_4_callback_t simple_callback)
{
    callback_setup.address_4_callback = simple_callback;
    callback_setup.address_4_callback_is_set = true;
    return ESP_OK;
}

esp_err_t set_callback_sequence_control(packet_library_sequence_control_callback_t simple_callback)
{
    callback_setup.sequence_control_callback = simple_callback;
    callback_setup.sequence_control_callback_is_set = true;
    return ESP_OK;
}

esp_err_t set_send_callback() // TODO
{
    return ESP_OK;
}

esp_err_t set_send_callback_frame_control() // TODO
{
    return ESP_OK;
}

esp_err_t set_send_callback_duration_id() // TODO
{
    return ESP_OK;
}

esp_err_t set_send_callback_address_1() // TODO
{
    return ESP_OK;
}

esp_err_t set_send_callback_address_2() // TODO
{
    return ESP_OK;
}

esp_err_t set_send_callback_address_3() // TODO
{
    return ESP_OK;
}

esp_err_t set_send_callback_address_4() // TODO
{
    return ESP_OK;
}

esp_err_t set_send_callback_sequence_control() // TODO
{
    return ESP_OK;
}
