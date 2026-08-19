
// Driver Configuration
const int NUM_DRIVERS = 100; // Number of drivers
unsigned int AnimationDelay = 40;   // Delay for animations in milliseconds

// Button Configuration
#define BUTTONS_ENABLED true 
#define NUM_BUTTONS 5 // Number of buttons used
// #define NUM_AMENITIES_DRIVERS 0 // Number of drivers used for amenities
#define DEFAULT_ON true // Default state of buttons on startup
const unsigned int DEBOUNCE_DELAY = 200;        // Debounce delay in milliseconds
const unsigned int DOUBLE_PRESS_INTERVAL = 400; // ms between two presses for a double press


// Bluetooth Configuration
#ifdef BLE
#define BLE_ENABLED true // Enable Bluetooth functionality
#else
#define BLE_ENABLED false // Disable Bluetooth functionality
#endif
static std::string device_name = "VU-IBS"; // Name of the ESP32 device for Bluetooth
static const char SERVICE_UUID[] = "41c89556-1739-417f-a5cb-c2573b0f6ea4";
static const char CHAR1_UUID[] = "2a368d84-e433-4963-bbce-8f2f08232bdf";
static const char CHAR_OTA_UUID[] = "dacbe98f-98ac-4f56-a2d3-157b3a3235ec";
static const char CHAR_NOTIFY_UUID[] = "ecc9918f-a46a-4047-ae4d-7e3a4fa4130f";


// ESP-NOW Configuration
uint8_t slaveAddress[] = {0xC4, 0x5B, 0xBE, 0xE9, 0xBA, 0x4C};

// ESP-NOW Broadcast Address
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};





// OTA Configuration
std::string WIFI_SSID = "Work";
std::string WIFI_PASS = "12345678";
static const char FIRMWARE_URL[] = "https://github.com/VUElectronic/ESP32_BIN_OTA/raw/refs/heads/main/IBS_Controller-V3.bin";