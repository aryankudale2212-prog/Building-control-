#define VERSION "v3.0"

#ifdef ESP32S3
// Button configuration
#define BUTTON1_PIN 17
#define BUTTON2_PIN 10

// LED indicators for buttons
#define BUTTON1_LED_PIN 18
#define BUTTON2_LED_PIN 8

// CAN configuration
#define CAN_TX_PIN 39
#define CAN_RX_PIN 40

// UART configuration
#define TXD_PIN 43
#define RXD_PIN 44

// TMC2209 configuration
#define TMC_TX_PIN 38
#define TMC_RX_PIN 48
#define TMC_ENABLE_PIN 21
#define TMC_STEP_PIN 12
#define TMC_DIR_PIN 13

#define LSW_HOME 9
#define LSW_END 11

#else
// TMC2209 configuration
#define TMC_RX_PIN 16
#define TMC_TX_PIN 17
#define TMC_ENABLE_PIN 15
#define TMC_STEP_PIN 27
#define TMC_DIR_PIN 14

#define LSW_HOME 13
#define LSW_END 26

#endif

// Interrupt Flags
volatile bool cmdInterrupt = false;
// volatile bool bleinterrupt = false;


// bool localOTA = false;

// #define OTALocal "OTA_Local"
// #define NoOTA "NoOTA"
// #define OTAStart "OTAStart"
// #define OTACompleted "OTACompleted"
// #define OTAInProgressStatus "OTAInProgress"
// #define OTAFailed "OTAFailed"

enum OTAStatus {
    NoOTA,
    OTAGit,
    OTALocal,
    OTACompleted,
    OTAInProgress,
    OTAFailed
};



// std::string OTAInProgress = NoOTA; // 0 - No OTA, 1 - OTA Start, 2 - OTA Completed, 3 - OTA in Progress, 4 - OTA Failed
volatile OTAStatus CurrentOTAStatus = NoOTA;
