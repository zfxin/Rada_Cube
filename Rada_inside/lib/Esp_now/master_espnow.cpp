#include "master_espnow.h"

Preferences espnow_master::prefs;

/**
 * @brief Verify whether both the left and right slave units' ESP-NOW MAC addresses are stored simultaneously within the NVS
 *         if NVS mac address stored and slave_mac is not set(all zero), then copy NVS mac address to slave_mac
 * @return true  - left_slave and right_slave MAC exist
 *         false - at list one MAC address is missing
 */
bool espnow_master::read_slave_mac()
{
    prefs.begin("slave",true);
    bool left_mac = prefs.isKey("left_slave_mac");
    if(left_mac)
    {
        if(memcmp(left_slave_mac, "\0\0\0\0\0\0", 6) == 0)
        {
            prefs.getBytes("left_slave_mac", left_slave_mac, 6);
        }
    }
    bool right_mac = prefs.isKey("right_slave_mac");
    if(right_mac)
    {
        if(memcmp(right_slave_mac, "\0\0\0\0\0\0", 6) == 0)
        {
            prefs.getBytes("right_slave_mac", right_slave_mac, 6);
        }
    }
    prefs.end();
    return left_mac && right_mac;
}

/**
 * @brief set WI-FI mode to STA and init ESP-NOW
 */
void espnow_master::wiif_init()
{
    WiFi.mode(WIFI_STA);
    if(esp_now_init() != ESP_OK)
    {
        printf("ESP-NOW init failed\r\n");
    }
    else
    {
        printf("ESP-NOW init success\r\n");
    }
}

/**
 * @brief broadcast mode init and register callback recv and send
 */
void espnow_master::broadcast_mode_init()
{
    wiif_init();
    WiFi.macAddress(_master_mac);
    printf("Master MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n",_master_mac[0],_master_mac[1],_master_mac[2],_master_mac[3],_master_mac[4],_master_mac[5]);
    memcpy(_peer_info.peer_addr,BROADCAST_ADDR,6);
    _peer_info.channel = 0;
    _peer_info.encrypt = false;
    esp_now_add_peer(&_peer_info);

    esp_now_register_recv_cb(broadcast_mode_recv);
    esp_now_register_send_cb(broadcast_mode_send_result);

    printf("Broadcast mode init success\r\n");
}

/**
 * @brief broadcast mode send master mac, wait for slave mac response, LED blink
 * when all slave mac received, stop blink and keep bright, then exit broadcast_mode
 * 
 */
void espnow_master::broadcast_mode_loop(uint8_t LED)
{

    pinMode(LED, OUTPUT);
    digitalWrite(LED, LOW);
    while(read_slave_mac() == false)
    {
        // broadcast master mac to slave
        esp_now_send(BROADCAST_ADDR, _master_mac, 6);
        printf("broadcast master mac to slave\r\n");
        digitalWrite(LED, HIGH);
        delay(1000);
        digitalWrite(LED, LOW);
        delay(1000);
    }
    // all slave mac reveived, stop blink and keep bright
    printf("All slave MAC have been received\r\n");
    digitalWrite(LED, HIGH);
    // Deregister broadcast mode
    esp_now_del_peer(BROADCAST_ADDR);
    esp_now_unregister_recv_cb();
    esp_now_unregister_send_cb();
}

/**
 * @brief printf broadcast mode send result
 */
void espnow_master::broadcast_mode_send_result(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    printf("%s\n", status == ESP_NOW_SEND_SUCCESS ? "Broadcast Send OK" : "Broadcast Send Fail");
}

/**
 * @brief broadcast mode recv callback
 */
void espnow_master::broadcast_mode_recv(const esp_now_recv_info_t *recv_info, const uint8_t* recv_data, int len)
{
    printf("Received data: %s\r\n", recv_data);
    if(memcmp(recv_data, "left_slave", 10) == 0)
    {
        const uint8_t* left_slave_mac = recv_info->src_addr;
        prefs.begin("slave", false);
        char left_slave_mac_str[18];
        sprintf(left_slave_mac_str, "%02X:%02X:%02X:%02X:%02X:%02X", left_slave_mac[0], left_slave_mac[1], left_slave_mac[2], left_slave_mac[3], left_slave_mac[4], left_slave_mac[5]);
        prefs.putString("left_slave_mac", left_slave_mac_str);
        prefs.end();
        printf("Left slave MAC saved\r\n");
        printf("Left slave MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n",left_slave_mac[0],left_slave_mac[1],left_slave_mac[2],left_slave_mac[3],left_slave_mac[4],left_slave_mac[5]);
    }
    else if(memcmp(recv_data, "right_slave", 11) == 0)
    {
        const uint8_t* right_slave_mac = recv_info->src_addr;
        prefs.begin("slave", false);
        char right_slave_mac_str[18];
        sprintf(right_slave_mac_str, "%02X:%02X:%02X:%02X:%02X:%02X", right_slave_mac[0], right_slave_mac[1], right_slave_mac[2], right_slave_mac[3], right_slave_mac[4], right_slave_mac[5]);
        prefs.putString("right_slave_mac", right_slave_mac_str);
        prefs.end();
        printf("Right slave MAC saved\r\n");
        printf("Right slave MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n",right_slave_mac[0],right_slave_mac[1],right_slave_mac[2],right_slave_mac[3],right_slave_mac[4],right_slave_mac[5]);
    }
}

/**
 * @brief Prior to entering broadcast mode, clear the slave namespace within the NVS and reset the slave variable to zero.
 */
void espnow_master::delete_slave_mac()
{
    // delete all slave namespace
    prefs.begin("slave", false);
    prefs.clear();
    prefs.end();

    // clear slave mac Variable and set to 0
    memset(left_slave_mac, 0, 6);
    memset(right_slave_mac, 0, 6);
    printf("Slave MAC has been deleted\r\n");
}


/**
 * @brief unicast mode add mac to esp_now
 */
void espnow_master::unicast_mac_add(uint8_t *mac_addr)
{
    esp_now_peer_info_t peer_info;
    memcpy(peer_info.peer_addr, mac_addr, 6);
    peer_info.channel = 0;
    peer_info.encrypt = false;
    esp_now_add_peer(&peer_info);
    printf("MAC: %02X:%02X:%02X:%02X:%02X:%02X has add\r\n", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
}

/**
 * @brief broadcast mode to unicast mode, init unicast mode and send match_success_ack
 */
void espnow_master::broadcast_mode_to_unicast_mode_init()
{
    unicast_mac_add(left_slave_mac);
    unicast_mac_add(right_slave_mac);
    esp_now_send(left_slave_mac, MATCH_SUCCESS_DATA, MATCH_SUCCESS_SIZE);
    esp_now_send(right_slave_mac, MATCH_SUCCESS_DATA, MATCH_SUCCESS_SIZE);
    esp_now_register_recv_cb(unicast_mode_recv);
}

/**
 * @brief unicast mode init and register callback 
 * send MATCH_SUCCESS_DATA to slave
 */
void espnow_master::unicast_mode_init()
{
    unicast_mac_add(left_slave_mac);
    unicast_mac_add(right_slave_mac);
    esp_now_register_recv_cb(unicast_mode_recv);
}

/**
 * @brief unicast mode recv callback
 */
void espnow_master::unicast_mode_recv(const esp_now_recv_info_t* recv_info, const uint8_t* recv_data, int len)
{

}


