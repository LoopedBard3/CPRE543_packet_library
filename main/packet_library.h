#ifndef PACKET_LIBRARY_H
#define PACKET_LIBRARY_H

static const char *LOGGING_TAG = "packet_library";

// TypeDefs
typedef struct {
    wifi_interface_t wifi_interface;
    bool wifi_interface_set;
} configuration_settings_t;

typedef struct {
    uint16_t frame_control;
    uint16_t duration_id;
    uint8_t address_1[6];
    uint8_t address_2[6];
    uint8_t address_3[6];
    uint16_t sequence_control;
    uint8_t address_4[6];
    uint8_t *payload;
} wifi_mac_data_frame_t;

typedef void (* packet_library_simple_callback_t)(wifi_mac_data_frame_t* packet);
typedef void (* packet_library_frame_control_callback_t)(uint16_t* frame_control);
typedef void (* packet_library_duration_id_callback_t)(uint16_t* duration_id);
typedef void (* packet_library_address_1_callback_t)(uint8_t* address_1);
typedef void (* packet_library_address_2_callback_t)(uint8_t* address_2);
typedef void (* packet_library_address_3_callback_t)(uint8_t* address_3);
typedef void (* packet_library_address_4_callback_t)(uint8_t* address_4);
typedef void (* packet_library_sequence_control_callback_t)(uint16_t* sequence_control);

typedef struct {
    bool general_callback_is_set;
    packet_library_simple_callback_t general_callback;
    bool frame_control_callback_is_set;
    packet_library_frame_control_callback_t frame_control_callback;
    bool duration_id_callback_is_set;
    packet_library_duration_id_callback_t duration_id_callback;
    bool address_1_callback_is_set;
    packet_library_address_1_callback_t address_1_callback;
    bool address_2_callback_is_set;
    packet_library_address_2_callback_t address_2_callback;
    bool address_3_callback_is_set;
    packet_library_address_3_callback_t address_3_callback;
    bool address_4_callback_is_set;
    packet_library_address_4_callback_t address_4_callback;
    bool sequence_control_callback_is_set;
    packet_library_sequence_control_callback_t sequence_control_callback;
    // bool payload_callback_is_set;
    // packet_library_simple_callback_t payload_callback;

} callback_setup_t;

// Setup/configuration methods
esp_err_t setup_wifi_simple(); // This or custom version have to be called
esp_err_t setup_wifi_custom(wifi_init_config_t config); 
esp_err_t setup_sta_default();
esp_err_t setup_promiscuous_default(wifi_promiscuous_cb_t callback);
esp_err_t setup_promiscuous_simple(); // Enables individual section callbacks
esp_err_t setup_promiscuous_simple_with_general_callback(packet_library_simple_callback_t simple_callback); 

// Send full control
esp_err_t send_packet_raw(const void* buffer, int length, bool en_sys_seq);
esp_err_t send_packet_simple(wifi_mac_data_frame_t* packet, int payload_length);

// Individual field callbacks/send options
esp_err_t set_receive_callback_frame_control(packet_library_frame_control_callback_t simple_callback);
esp_err_t set_receive_callback_duration_id(packet_library_duration_id_callback_t simple_callback);
esp_err_t set_receive_callback_address_1(packet_library_address_1_callback_t simple_callback);
esp_err_t set_receive_callback_address_2(packet_library_address_2_callback_t simple_callback);
esp_err_t set_receive_callback_address_3(packet_library_address_3_callback_t simple_callback);
esp_err_t set_receive_callback_address_4(packet_library_address_4_callback_t simple_callback);
esp_err_t set_receive_callback_sequence_control(packet_library_sequence_control_callback_t simple_callback);
esp_err_t remove_receive_callback_frame_control();
esp_err_t remove_receive_callback_duration_id();
esp_err_t remove_receive_callback_address_1();
esp_err_t remove_receive_callback_address_2();
esp_err_t remove_receive_callback_address_3();
esp_err_t remove_receive_callback_address_4();
esp_err_t remove_receive_callback_sequence_control();
// TODO: add payload callback/send, will need to include data length stuff

esp_err_t set_send_callback_frame_control(packet_library_frame_control_callback_t simple_callback);
esp_err_t set_send_callback_duration_id(packet_library_duration_id_callback_t simple_callback);
esp_err_t set_send_callback_address_1(packet_library_address_1_callback_t simple_callback);
esp_err_t set_send_callback_address_2(packet_library_address_2_callback_t simple_callback);
esp_err_t set_send_callback_address_3(packet_library_address_3_callback_t simple_callback);
esp_err_t set_send_callback_address_4(packet_library_address_4_callback_t simple_callback);
esp_err_t set_send_callback_sequence_control(packet_library_sequence_control_callback_t simple_callback);
esp_err_t remove_send_callback_frame_control();
esp_err_t remove_send_callback_duration_id();
esp_err_t remove_send_callback_address_1();
esp_err_t remove_send_callback_address_2();
esp_err_t remove_send_callback_address_3();
esp_err_t remove_send_callback_address_4();
esp_err_t remove_send_callback_sequence_control();

#endif