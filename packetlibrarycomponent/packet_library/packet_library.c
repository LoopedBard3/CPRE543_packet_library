#include "packet_library.h"

// Private helper static types
static callback_setup_t promisc_callback_setup; // This type manages the callback pointers for the general and individual callbacks along with other helper values that only the component needs to worry about.
static callback_setup_t send_callback_setup; // This type manages the callback pointers for the general and individual callbacks along with other helper values that only the component needs to worry about.
static configuration_settings_t configuration_holder; // This is a general configuration holder that handles information like what wifi interface is being used, the devices MAC, and whether the device is connected to an AP.

/* 
    This is the callback the component uses to provide general and field specific callbacks to the code using it.
    It is the callback that is passed to the underlying ESP-IDF API for received packet callback.
    The general flow is to get the packet data we need for the component provided callbacks, running the general
    callback, and running the individual field callbacks.
*/
static void promisc_simple_callback(void *buf, wifi_promiscuous_pkt_type_t type)
{
    const wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
    int payload_length = pkt->rx_ctrl.sig_len - sizeof(wifi_promiscuous_pkt_t);
    wifi_mac_data_frame_t *frame = (wifi_mac_data_frame_t *)pkt->payload;

    if(promisc_callback_setup.precallback_print != DISABLE)
    {
        ESP_LOGI(LOGGING_TAG, "PROM PRECALL START");
        if(promisc_callback_setup.precallback_print == ANNOTATED)
        {
            log_packet_annotated(frame, payload_length, LOGGING_TAG);
        } 
        else if(promisc_callback_setup.precallback_print == HEX)
        {
            log_packet_hex(frame, payload_length, LOGGING_TAG);
        }
        ESP_LOGI(LOGGING_TAG, "PROM PRECALL END");
    }

    // Do the simple callback and then each individual callback action
    if (promisc_callback_setup.general_callback_is_set)
    {
        promisc_callback_setup.general_callback(frame, payload_length);
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
        ESP_LOGI(LOGGING_TAG, "PROM POSTCALL PRINT START");
        if(promisc_callback_setup.postcallback_print == ANNOTATED)
        {
            log_packet_annotated(frame, payload_length, LOGGING_TAG);
        } 
        else if(promisc_callback_setup.postcallback_print == HEX)
        {
            log_packet_hex(frame, payload_length, LOGGING_TAG);
        }
        ESP_LOGI(LOGGING_TAG, "PROM POSTCALL PRINT END");
    }
}

// **************************************************
// Setup/Configuration Functions
// **************************************************
// This initializes the ESP-32 WiFi systems to setup as a station, using the default Wifi Config provided by ESP-IDF
esp_err_t setup_wifi_station_simple() // LOC (Lines of Code saved): 1 + 10 = 11
{
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    return setup_wifi_custom(cfg, true);
}

// This initializes the ESP-32 WiFi systems to setup as an access point, using the default Wifi Config provided by ESP-IDF
esp_err_t setup_wifi_access_point_simple() // LOC: 1 + 10 = 11
{
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    return setup_wifi_custom(cfg, false);
}

// This initializes the WiFi system using a custom user supplied config and whether the ESP-32 is going to act as a station or access point
esp_err_t setup_wifi_custom(wifi_init_config_t config, bool as_station) // LOC: 10
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *netif;
    if(as_station)
    {
        netif = esp_netif_create_default_wifi_sta();
    }
    else
    {
        netif = esp_netif_create_default_wifi_ap();
    }
    assert(netif);
    ESP_ERROR_CHECK(esp_wifi_init(&config));
    ESP_ERROR_CHECK(esp_wifi_get_mac(WIFI_IF_STA, configuration_holder.mac_addr));
    ESP_LOGI(LOGGING_TAG, "WIFI INITIALIZED");
    return ESP_OK;
}

// This sets the ESP-32 WiFi mode to Station and enables the WiFi system. 
esp_err_t setup_sta_default() // LOC: 2
{
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    configuration_holder.wifi_interface = WIFI_IF_STA;
    configuration_holder.wifi_interface_set = true;
    return ESP_OK;
}

// This acts as a wrapper around the build in packet received filter in order to keep consistent naming thorughout more of the component.
esp_err_t setup_packets_type_filter(const wifi_promiscuous_filter_t *type_filter) // LOC: 1
{ 
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_filter(type_filter));
    return ESP_OK;
}

// This sets up the packet reception capabilities (promiscuous enabled) and allows the user to set their own callback to run outside the component managed system.
esp_err_t setup_promiscuous_custom(wifi_promiscuous_cb_t callback) // LOC: 2
{
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(callback));
    ESP_LOGI(LOGGING_TAG, "DEFAULT PROMISCUOUS CALLBACK SET");
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
    ESP_LOGI(LOGGING_TAG, "DEFAULT PROMISCUOUS STARTED");
    return ESP_OK;
}

// This enables the the packet reception capabilities (promiscuous mode) along with setting the callback to be the component managed callback near the top of this source.
esp_err_t setup_promiscuous_simple() // LOC: 2
{
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(&promisc_simple_callback));
    ESP_LOGI(LOGGING_TAG, "SIMPLE PROMISCUOUS CALLBACK SET");
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
    ESP_LOGI(LOGGING_TAG, "SIMPLE PROMISCUOUS STARTED");
    return ESP_OK;
}

// This enables the packet reception capabilities similar to 'setup_promiscuous_simple', but also sets a general callback up for use in the components managed callback system. 
esp_err_t setup_promiscuous_simple_with_general_callback(packet_library_simple_callback_t simple_callback) // LOC: 0 + 2 = 2
{
    promisc_callback_setup.general_callback = simple_callback;
    promisc_callback_setup.general_callback_is_set = true;
    return setup_promiscuous_simple();
}

// This disables the general callback
esp_err_t disable_promiscuous_general_callback() // LOC: 0
{
    promisc_callback_setup.general_callback_is_set = false;
    return ESP_OK;
}

// This allows for the manual toggling of the promiscuous packet reception, and therefore the packet received callback running
esp_err_t set_promiscuous_enabled(bool enable) // LOC: 0
{
    return esp_wifi_set_promiscuous(enable);
}

// This sets the ESP-32 up for use to both send and receive packets using the system managed received packet system, along with getting and storing the devices mac_address for other methods
esp_err_t setup_sta_and_promiscuous_simple() // LOC: 5
{
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(&promisc_simple_callback));
    configuration_holder.wifi_interface = WIFI_IF_STA;
    configuration_holder.wifi_interface_set = true;
    ESP_ERROR_CHECK(esp_wifi_get_mac(WIFI_IF_STA, configuration_holder.mac_addr));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
    return ESP_OK;
}

// This sets the general promiscuous callback up and then sets up the ESP-32 for station+promiscuous use via 'setup_sta_and_promiscuous_simple'.
esp_err_t setup_sta_and_promiscuous_simple_with_promisc_general_callback(packet_library_simple_callback_t simple_callback) // LOC: 0 + 5
{
    promisc_callback_setup.general_callback = simple_callback;
    promisc_callback_setup.general_callback_is_set = true;
    return setup_sta_and_promiscuous_simple();
}

// This sets up the ESP-32 device so that it acts as an access point based on the passed in configuration
esp_err_t setup_wpa_ap(wifi_ap_config_t ap_configuration) // LOC: 6
{
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    configuration_holder.wifi_interface = WIFI_IF_AP;
    configuration_holder.wifi_interface_set = true;
    wifi_config_t config = {
        .ap = ap_configuration
    };
    esp_wifi_set_protocol(configuration_holder.wifi_interface, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);
    ESP_ERROR_CHECK(esp_wifi_set_config(configuration_holder.wifi_interface, &config));
    ESP_ERROR_CHECK(esp_wifi_get_mac(WIFI_IF_AP, configuration_holder.mac_addr));
    ESP_ERROR_CHECK(esp_wifi_start());
    return ESP_OK;
}

// This sets up the ESP-32 device so that it acts as a station and connect to the access point based on the passed in configuration
esp_err_t setup_wpa_sta(wifi_sta_config_t station_connection_configuration) // LOC: 15
{
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    configuration_holder.wifi_interface = WIFI_IF_STA;
    configuration_holder.wifi_interface_set = true;
    wifi_config_t config = {    
        .sta = station_connection_configuration
    };
    esp_wifi_set_protocol(configuration_holder.wifi_interface, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);
    ESP_ERROR_CHECK(esp_wifi_set_config(configuration_holder.wifi_interface, &config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());

    esp_err_t status = ESP_ERR_WIFI_NOT_CONNECT;
    int connection_test_counter = 0;
    const TickType_t xDelay = 100 / portTICK_PERIOD_MS; 
    while(status == ESP_ERR_WIFI_NOT_CONNECT && connection_test_counter < 20)
    {
        vTaskDelay( xDelay );
        ESP_LOGI(LOGGING_TAG, "CONNECTING TO AP ATTEMPT %d", connection_test_counter);
        status = esp_wifi_sta_get_ap_info(&configuration_holder.connected_ap_record);
        connection_test_counter++;
    }
    if(status == ESP_OK)
    {
        configuration_holder.wifi_connected_to_ap = true;
        return ESP_OK;
    }
    return status;
}

// **************************************************
// Send Packet Methods
// **************************************************
// Sends a packet without any of the structuring provided by the component (raw)
esp_err_t send_packet_raw_no_callback(const void* buffer, int length, bool en_sys_seq) // LOC: 1
{
    if(configuration_holder.wifi_interface_set != true)
    {
        return ESP_ERR_WIFI_IF;
    }
    esp_wifi_80211_tx(configuration_holder.wifi_interface, buffer, length, en_sys_seq);
    return ESP_OK;
}

// Sends a packet conforming to the component provided wifi_mac_data_frame_t, along with running all enabled callbacks on the packet before sending.
esp_err_t send_packet_simple(wifi_mac_data_frame_t* packet, int payload_length) // LOC: 12 (Counted check at top, callback execution lines, and send) 
{
    int length = sizeof(wifi_mac_data_frame_t) + payload_length;
    if(configuration_holder.wifi_interface_set != true)
    {
        return ESP_ERR_WIFI_IF;
    }

    if(send_callback_setup.precallback_print != DISABLE)
    {
        ESP_LOGI(LOGGING_TAG, "SEND PRECALL START");
        if(send_callback_setup.precallback_print == ANNOTATED)
        {
            log_packet_annotated(packet, payload_length, LOGGING_TAG);
        } 
        else if(send_callback_setup.precallback_print == HEX)
        {
            log_packet_hex(packet, payload_length, LOGGING_TAG);
        }
        ESP_LOGI(LOGGING_TAG, "SEND PRECALL END");
    }

    // Do the simple callback and then each individual callback action
    if (send_callback_setup.general_callback_is_set)
    {
        send_callback_setup.general_callback(packet, payload_length);
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
        ESP_LOGI(LOGGING_TAG, "SEND POSTCALL START");
        if(send_callback_setup.postcallback_print == ANNOTATED)
        {
            log_packet_annotated(packet, payload_length, LOGGING_TAG);
        } 
        else if(send_callback_setup.postcallback_print == HEX)
        {
            log_packet_hex(packet, payload_length, LOGGING_TAG);
        }
        ESP_LOGI(LOGGING_TAG, "SEND POSTCALL END");
    }

    esp_wifi_80211_tx(configuration_holder.wifi_interface, (void *)packet, length, true);
    return ESP_OK;
}

// This is an AP helper method to send a packet to a specific station, based on the target stations MAC address, with the component manages everything but the payload and finding the target stations MAC addr
esp_err_t send_payload_ap_to_station(uint8_t payload[], int payload_length, uint8_t station_addr[6]) // LOC: 5 + 10 + 12 = 27 (alloc_packet_custom counted as 10)
{
    if(configuration_holder.wifi_interface_set == false)
    {
        return ESP_ERR_WIFI_NOT_INIT;
    }
    if(configuration_holder.wifi_interface != WIFI_IF_AP)
    {
        return ESP_ERR_WIFI_MODE;
    }
    wifi_mac_data_frame_t* pkt = alloc_packet_custom(
        0x0208, // Always send as a data packet, also set To DS 0/From DS 1.
        0xFA, // Set the duration ID
        station_addr,
        configuration_holder.mac_addr,
        configuration_holder.mac_addr,
        0x00, // This is managed by the chip and gets overwritten on simple send
        (uint8_t []){ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
        payload_length,
        payload
    );
    send_packet_simple(pkt, payload_length);
    free(pkt);
    return ESP_OK;
}

// This is an AP helper method to broadcast a packet, with the component manages everything but the payload
esp_err_t send_payload_ap_broadcast(uint8_t payload[], int payload_length) // LOC: 1 + 27 = 28
{
    return send_payload_ap_to_station(payload, payload_length, (uint8_t []){ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
} 

// This is a station helper method for when connected to an AP for sending a payload to the AP, with the component managing the MAC address fiels and packet allocation.
esp_err_t send_payload_sta_to_access_point(uint8_t payload[], int payload_length) // LOC: 5 + 10 + 12 = 27
{
    if(configuration_holder.wifi_interface_set == false || configuration_holder.wifi_connected_to_ap == false)
    {
        return ESP_ERR_WIFI_NOT_INIT;
    }
    if(configuration_holder.wifi_interface != WIFI_IF_STA)
    {
        return ESP_ERR_WIFI_MODE;
    }
    wifi_mac_data_frame_t* pkt = alloc_packet_custom(
        0x0108, // Always send as a data packet, also set To DS 1/From DS 0
        0xFA, // Set the duration ID
        configuration_holder.connected_ap_record.bssid,
        configuration_holder.mac_addr,
        configuration_holder.connected_ap_record.bssid,
        0x00, // This is managed by the chip and gets overwritten on simple send
        (uint8_t []){ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
        payload_length,
        payload
    );
    send_packet_simple(pkt, payload_length);
    free(pkt);
    return ESP_OK;
}

// This is a station helper method for sending a packet to a target MAC through the connected access point, with the component managing the AP and this ESP-32 address fields, and packet allocation.
esp_err_t send_payload_sta_through_access_point(uint8_t payload[], int payload_length, uint8_t target_mac[6]) // LOC: 5 + 10 + 12 = 27
{
    if(configuration_holder.wifi_interface_set == false || configuration_holder.wifi_connected_to_ap == false)
    {
        return ESP_ERR_WIFI_NOT_INIT;
    }
    if(configuration_holder.wifi_interface != WIFI_IF_STA)
    {
        return ESP_ERR_WIFI_MODE;
    }
    wifi_mac_data_frame_t* pkt = alloc_packet_custom(
        0x0108, // Always send as a data packet, also set To DS 1/From DS 0
        0xFA, // Set the duration ID
        configuration_holder.connected_ap_record.bssid,
        configuration_holder.mac_addr,
        target_mac,
        0x00, // This is managed by the chip and gets overwritten on simple send
        (uint8_t []){ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
        payload_length,
        payload
    );
    send_packet_simple(pkt, payload_length);
    free(pkt);
    return ESP_OK;
}

// **************************************************
// Receive Callback Methods
// All the methods in this section are for managing (enabling/adding and disabling the callbacks for sending packets)
// Set methods add and enable the callback specified in the method name and the remove methods disable them.
// **************************************************
esp_err_t set_receive_callback_general(packet_library_simple_callback_t simple_callback)
{
    promisc_callback_setup.general_callback = simple_callback;
    promisc_callback_setup.general_callback_is_set = true;
    return ESP_OK;
}

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

esp_err_t remove_receive_callback_general()
{
    promisc_callback_setup.general_callback_is_set = false;
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
// All the methods in this section are for managing (enabling/adding and disabling the callbacks for sending packets)
// Set methods add and enable the callback specified in the method name and the remove methods disable them.
// **************************************************

esp_err_t set_send_callback_general(packet_library_simple_callback_t simple_callback)
{
    send_callback_setup.general_callback = simple_callback;
    send_callback_setup.general_callback_is_set = true;
    return ESP_OK;
}

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

esp_err_t remove_send_callback_general()
{
    send_callback_setup.general_callback_is_set = false;
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

// This logs the given packet with annotations to help separate the specific values inside the packet.
esp_err_t log_packet_annotated(wifi_mac_data_frame_t* packet, int payload_length, const char * TAG) // LOC: 15
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
        // Generate the payload string
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

// This logs the given packet as plain hex, no annotations, although the payload is on a different line from the static information.
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
        // Generate the payload string
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

// Helper method for creating/allocation a packet to send. This takes in values for each of the static fields and the payload, turning packet generation from a multi-line allocation into a single line method call.
// The generated packet must be free'd upon completion of its use
wifi_mac_data_frame_t* alloc_packet_custom(uint16_t frame_control, uint16_t duration_id, uint8_t address_1[6], uint8_t address_2[6], uint8_t address_3[6], uint16_t sequence_control, uint8_t address_4[6], int payload_length, uint8_t* payload) // LOC: 10
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

// This allocates a packet while only setting the payload (all other values are set to zero). From here, any individual value can be modified as desired
wifi_mac_data_frame_t* alloc_packet_default_payload(int payload_length, uint8_t *payload)
{
    return alloc_packet_custom(0, 0, (uint8_t []){0,0,0,0,0,0}, (uint8_t []){0,0,0,0,0,0}, (uint8_t []){0,0,0,0,0,0}, 0, (uint8_t []){0,0,0,0,0,0}, payload_length, payload);
}

// This allocates a packet while setting up space for a payload, but not actually setting it to have any data.
wifi_mac_data_frame_t* alloc_packet_default(int payload_length)
{
    return alloc_packet_default_payload(payload_length, NULL);
}

// Helper method to get this ESP-32's MAC address and puts it in the mac_output_holder
esp_err_t get_current_mac(uint8_t mac_output_holder[6])
{
    // Return mac_addr if the ESP-32 is configured properly
    if(!configuration_holder.wifi_connected_to_ap && configuration_holder.mac_addr[0] != 0 && configuration_holder.mac_addr[1] != 0 && configuration_holder.mac_addr[2] != 0 && configuration_holder.mac_addr[3] != 0 && configuration_holder.mac_addr[4] != 0 && configuration_holder.mac_addr[5] != 0)
    {
        mac_output_holder[0] = configuration_holder.mac_addr[0];
        mac_output_holder[1] = configuration_holder.mac_addr[1];
        mac_output_holder[2] = configuration_holder.mac_addr[2];
        mac_output_holder[3] = configuration_holder.mac_addr[3];
        mac_output_holder[4] = configuration_holder.mac_addr[4];
        mac_output_holder[5] = configuration_holder.mac_addr[5];

        return ESP_OK;
    }
    return ESP_ERR_WIFI_NOT_INIT;
}

// Helper method to get the MAC address of the access point the ESP-32 is connected to and puts it in the mac_output_holder
esp_err_t get_current_ap_mac(uint8_t mac_output_holder[6])
{
    // Return mac_addr if it is configured properly
    if(!configuration_holder.wifi_connected_to_ap && configuration_holder.connected_ap_record.bssid[0] != 0 && configuration_holder.connected_ap_record.bssid[1] != 0 && configuration_holder.connected_ap_record.bssid[2] != 0 && configuration_holder.connected_ap_record.bssid[3] != 0 && configuration_holder.connected_ap_record.bssid[4] != 0 && configuration_holder.connected_ap_record.bssid[5] != 0)
    {
        mac_output_holder[0] = configuration_holder.connected_ap_record.bssid[0];
        mac_output_holder[1] = configuration_holder.connected_ap_record.bssid[1];
        mac_output_holder[2] = configuration_holder.connected_ap_record.bssid[2];
        mac_output_holder[3] = configuration_holder.connected_ap_record.bssid[3];
        mac_output_holder[4] = configuration_holder.connected_ap_record.bssid[4];
        mac_output_holder[5] = configuration_holder.connected_ap_record.bssid[5];

        return ESP_OK;
    }
    return ESP_ERR_WIFI_NOT_INIT;
}

// Helper method to get the MAC addresses of the connected stations when acting as an access point
// The connected MACs are stored in 'station_macs_holder' and the number of stations is stored in 'number_valid_stations_holder'
esp_err_t get_current_ap_connected_sta_macs(uint8_t station_macs_holder[10][6], int* number_valid_stations_holder) // Most stations anyways is 10, very little storage so get 10 always anyways // LOC: 15
{
    if(configuration_holder.wifi_interface_set == false)
    {
        return ESP_ERR_WIFI_NOT_INIT;
    }
    if(configuration_holder.wifi_interface != WIFI_IF_AP)
    {
        return ESP_ERR_WIFI_MODE;
    }
    wifi_sta_list_t sta_list_holder;
    ESP_ERROR_CHECK(esp_wifi_ap_get_sta_list(&sta_list_holder));
    int counter = 0;
    for(counter = 0; counter < sta_list_holder.num; counter++)
    {
        station_macs_holder[counter][0] = sta_list_holder.sta[counter].mac[0];
        station_macs_holder[counter][1] = sta_list_holder.sta[counter].mac[1];
        station_macs_holder[counter][2] = sta_list_holder.sta[counter].mac[2];
        station_macs_holder[counter][3] = sta_list_holder.sta[counter].mac[3];
        station_macs_holder[counter][4] = sta_list_holder.sta[counter].mac[4];
        station_macs_holder[counter][5] = sta_list_holder.sta[counter].mac[5];
    }
    *number_valid_stations_holder = sta_list_holder.num;
    return ESP_OK;
}