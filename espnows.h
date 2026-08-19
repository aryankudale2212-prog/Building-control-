#include <esp_now.h>

typedef struct struct_message
{
    char cmd[256];
} struct_message;

struct_message incomingData;
struct_message outgoingData;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t sendStatus)
{
    DEBUG_SERIAL.print("Last Packet Send Status: ");
    DEBUG_SERIAL.println(sendStatus == 0 ? "Delivery Success" : "Delivery Fail");
}

void onDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len)
{
    memcpy(&incomingData, data, sizeof(incomingData));
    DEBUG_SERIAL.print("Received data from: ");
    ParsedBTCommand = parse_command(incomingData.cmd);  
    if (ParsedBTCommand.command == "MT")
    {
        }
    // if (ParsedBTCommand.valid) {
    //   cmdInterrupt = true;
    // }
}

void ESPNOWSetup()
{
    #ifndef ESPNOWS 
    return;
    #endif
    // Print device MAC address
    DEBUG_SERIAL.print("Device MAC: ");
    DEBUG_SERIAL.println(WiFi.macAddress());

    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK)
    {
        DEBUG_SERIAL.println("Error initializing ESP-NOW");
        return;
    }
    esp_now_register_send_cb(OnDataSent);
    esp_now_register_recv_cb(onDataRecv);

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK)
    {
        DEBUG_SERIAL.println("Failed to add peer");
        return;
    }
}

void ESPNOWSendMessage(const char* message)
{
    #ifndef ESPNOWS 
    return;
    #endif
    strncpy((char*)outgoingData.cmd, message, sizeof(outgoingData.cmd) - 1);
    outgoingData.cmd[sizeof(outgoingData.cmd) - 1] = '\0';
    
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &outgoingData, sizeof(outgoingData));
    
    if (result == ESP_OK) {
        DEBUG_SERIAL.println("Broadcast message sent successfully");
    } else {
        DEBUG_SERIAL.println("Error sending broadcast message");
    }
}