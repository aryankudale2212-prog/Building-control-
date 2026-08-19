#include <TMCStepper.h>

#define R_SENSE 0.11f
#define DRIVER_ADDR 0b00

#define MIN_STEP_DELAY 100
#define MAX_STEP_DELAY 5000

int stepDelay = 300;          // in microseconds
uint8_t StallThreshold = 200; // SG threshold for stall detection

#define TMC_MICROSTEPS 8
#define MAX_TMC_CURRENT 2000 // in mA

// #define STALL_VALUE 100    // [0..255]
// #define DIRECTION_CHANGE_DELAY 1000 // 1 second delay after direction change

bool running = false;
bool runningContinuously = false;
uint32_t stallGuardCounter = 0;
uint32_t lastStallCheckTime = 0;
#define STALL_CHECK_INTERVAL 50 // Check every 50ms instead of 5ms

volatile uint8_t stepPin_state = 0; // Track step pin state to avoid reading
volatile unsigned int stepCount = 0;

TMC2209Stepper driver(&Serial1, R_SENSE, DRIVER_ADDR);

// ESP32 timer variables
hw_timer_t *timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

// Timer ISR
void IRAM_ATTR onTimer()
{
    if (stepCount == 0)
    {
        timerAlarmDisable(timer);
        gpio_set_level(gpio_num_t(TMC_ENABLE_PIN), 1);
        gpio_set_level(gpio_num_t(TMC_STEP_PIN), 0);
        // stallGuardCounter = 0;
        running = false;
        return;
    }

    stepPin_state ^= 1;
    gpio_set_level(gpio_num_t(TMC_STEP_PIN), stepPin_state);

    stepCount--;
}

void motorInit()
{
#ifndef MOT
    return;
#endif
    //   DEBUG_SERIAL.begin(115200);
    Serial1.begin(115200, SERIAL_8N1, TMC_RX_PIN, TMC_TX_PIN);

    pinMode(TMC_ENABLE_PIN, OUTPUT);
    pinMode(TMC_STEP_PIN, OUTPUT);
    pinMode(TMC_DIR_PIN, OUTPUT);
    digitalWrite(TMC_ENABLE_PIN, HIGH); // Disable motor initially
    digitalWrite(TMC_STEP_PIN, LOW);    // Start with step pin low
    digitalWrite(TMC_DIR_PIN, LOW);     // Default direction

    pinMode(LSW_HOME, INPUT_PULLUP);
    pinMode(LSW_END, INPUT_PULLUP);
    // Attach ISR to limit switches
    attachInterrupt(digitalPinToInterrupt(LSW_HOME), []()
                    {
        if (running)
        {
        running = false;
        timerAlarmDisable(timer);
        gpio_set_level(gpio_num_t(TMC_ENABLE_PIN), 1);
        stallGuardCounter = 0;
        } }, FALLING);
    attachInterrupt(digitalPinToInterrupt(LSW_END), []()
                    {
        if (running)
        {
        running = false;
        timerAlarmDisable(timer);
        gpio_set_level(gpio_num_t(TMC_ENABLE_PIN), 1);
        stallGuardCounter = 0;
        } }, FALLING);

    DEBUG_SERIAL.println("TMC2209 Initialization");

    driver.begin();
    driver.toff(4);
    driver.blank_time(24);
    driver.rms_current(500); // mA
    driver.microsteps(TMC_MICROSTEPS);
    driver.TCOOLTHRS(0xFFFFF); // 20bit max
    driver.semin(5);
    driver.semax(2);
    driver.sedn(0b01);
    driver.SGTHRS(StallThreshold);
    driver.en_spreadCycle();
    delay(500);

    if (driver.test_connection() == 0)
        DEBUG_SERIAL.println("TMC2209 connected.");
    else
        DEBUG_SERIAL.println("Connection failed!");

    //   digitalWrite(TMC_DIR_PIN, direction);

    // Set up ESP32 timer interrupt
    timer = timerBegin(0, 80, true);             // Timer 0, prescaler 80, count up
    timerAttachInterrupt(timer, &onTimer, true); // Edge trigger
    timerAlarmWrite(timer, stepDelay, true);     // Initial period value
    timerAlarmDisable(timer);
}

void changeDirection(bool direction)
{
    digitalWrite(TMC_DIR_PIN, direction);
    delay(200); // Give motor time to settle
}

bool setMotorSpeed(int speedUs, int stall_threshold)
{
    if (speedUs < MIN_STEP_DELAY || speedUs > MAX_STEP_DELAY || stall_threshold < 1 || stall_threshold > 255)
        return false;
    stepDelay = speedUs;
    if (timer != NULL)
    {
        timerAlarmWrite(timer, stepDelay, true);
    }
    StallThreshold = stall_threshold;
    driver.SGTHRS(StallThreshold);
    DEBUG_SERIAL.printf("Motor speed: %d us, Stall threshold: %d\n", stepDelay, StallThreshold);
    return true;
}

void MotorHandle(int direction, int steps = 0, int currentmA = 0)
{
    if (timer == NULL)
    {
        DEBUG_SERIAL.println("Error: Timer not initialized");
        return;
    }

    if (steps == 0)
    {
        // Continuous mode
        runningContinuously = true;
        steps = 0xFFFFFFFF; // Large number to simulate continuous run
    }
    else
    {
        runningContinuously = false;
    }
    stepCount = steps;

    // Apply custom speed & stall threshold if provided and valid
    // setMotorSpeed(speedUs, StallThreshold);

    if (currentmA > 99 && currentmA <= MAX_TMC_CURRENT) // TMC2209 max ~2A
    {
        driver.rms_current(currentmA);
        DEBUG_SERIAL.printf("Motor current set to %d mA\n", currentmA);
    }

    if (direction == 1 || direction == 2)
    {
        if (running)
            timerAlarmDisable(timer); // Only disable if already running
        running = true;
        stallGuardCounter = 0; // Reset stall detection
        bool dir = (direction == 1) ? 0 : 1;
        changeDirection(dir);
        digitalWrite(TMC_ENABLE_PIN, LOW);
        timerAlarmEnable(timer);
        DEBUG_SERIAL.printf("Motor started in %s direction\n", dir ? "Backward" : "Forward");
    }
    else
    {
        if (!running)
            return; // Already stopped

        running = false;
        timerAlarmDisable(timer);
        digitalWrite(TMC_ENABLE_PIN, HIGH);
        stallGuardCounter = 0;

        DEBUG_SERIAL.println("Motor stopped");
    }
}
void stallGuardHandler()
{
    if (!running)
        return;

    uint32_t currentTime = millis();
    if (currentTime - lastStallCheckTime < STALL_CHECK_INTERVAL)
        return; // Skip if not enough time has passed

    lastStallCheckTime = currentTime;

    uint16_t sg_result = driver.SG_RESULT();
    DEBUG_SERIAL.print("SG: ");
    DEBUG_SERIAL.println(sg_result);

    // If StallGuard reading is below threshold, consider it as obstacle detected
    if (sg_result < StallThreshold)
    {
        stallGuardCounter++;
        if (stallGuardCounter > 2)
        {
            DEBUG_SERIAL.println("Obstacle detected! Stopping motor...");
            MotorHandle(0); // Stop motor
            stallGuardCounter = 0;
        }
    }
    else
    {
        stallGuardCounter = 0; // Reset counter if no stall detected
    }
}

void stepsCalibration()
{
    const unsigned int testMaxSteps = 80000;
    const int testSpeedUs = 200; // 200 microseconds
    const int testStallThreshold = 200;

    DEBUG_SERIAL.println("Starting steps calibration...");

    // Set test speed and stall threshold
    setMotorSpeed(testSpeedUs, testStallThreshold);

    // Start motor in forward direction for testSteps
    MotorHandle(2, testMaxSteps);

    // Wait until motor stops
    // while (running)
    // {
    //     stallGuardHandler();
    //     delay(10);
    // }

    // DEBUG_SERIAL.println("Steps calibration completed.");
}