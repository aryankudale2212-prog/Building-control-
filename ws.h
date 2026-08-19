
#include <FastLED.h>

uint8_t ON_Value = 0; // Inverted logic, 0 means ON
uint8_t OFF_Value = 255;

// FastLED configuration
#ifdef ESP32S3
  #define DATA_PIN 47
#else
#define DATA_PIN 2
#endif
#define LED_TYPE WS2811 // Type of LED strip

CRGB Drivers[NUM_DRIVERS];
int total_channels = (NUM_DRIVERS * 3) - 1;
// int animation_channels = total_channels-3;

// // Animation settings
// unsigned int AnimationDelay = 40;   // Delay for animations in milliseconds
unsigned int RUNNING_ANIMATION = 0; // Current running animation number

bool interrupt() // BLE and Button interrupt handler (return true if interrupted)
{
  if (cmdInterrupt)
  {
    RUNNING_ANIMATION = 0;
    return true;
  }
  return false;
}

#define CHECK_INTERRUPT() \
  if (interrupt())        \
  return

void WSsetup()
{
  FastLED.addLeds<LED_TYPE, DATA_PIN, RGB>(Drivers, NUM_DRIVERS);
  FastLED.clear(true); // Clear all LEDs initially
}

void TurnONOFF(bool on, int startindex, int endindex)
{
  if (startindex < 0 || endindex > total_channels || startindex > endindex)
  {
    DEBUG_SERIAL.printf("Invalid driver index range: %d to %d\n", startindex, endindex);
    return;
  }

  uint8_t brightness = on ? ON_Value : OFF_Value; // Set brightness to 255 if on, otherwise 0

  for (int index = startindex; index <= endindex; index++)
  {
    int driverIndex = index / 3;  // Each driver has 3 channels (R, G, B)
    int channelIndex = index % 3; // 0 for R, 1 for G, 2 for B
    if (driverIndex >= NUM_DRIVERS)
    {
      DEBUG_SERIAL.printf("Invalid driver index: %d\n", driverIndex);
      continue;
    }
    if (channelIndex == 0)
    {
      Drivers[driverIndex].r = brightness; // Turn on Red channel
    }
    else if (channelIndex == 1)
    {
      Drivers[driverIndex].g = brightness; // Turn on Green channel
    }
    else if (channelIndex == 2)
    {
      Drivers[driverIndex].b = brightness; // Turn on Blue channel
    }
  }
  FastLED.show(); // Update the LEDs
  DEBUG_SERIAL.printf("%s : %d to %d\n", on ? "ON" : "OFF", startindex, endindex);
  RUNNING_ANIMATION = 0;
}

void TurnDriverONOFF(bool on, int driverIndex)
{
  if (driverIndex < 0 || driverIndex >= NUM_DRIVERS)
  {
    DEBUG_SERIAL.printf("Invalid driver index: %d\n", driverIndex);
    return;
  }

  uint8_t brightness = on ? ON_Value : OFF_Value; // Set brightness to 255 if on, otherwise 0

  Drivers[driverIndex] = CRGB(brightness, brightness, brightness); // Set all channels of the driver

  FastLED.show(); // Update the LEDs
  DEBUG_SERIAL.printf("%s Driver %d\n", on ? "ON" : "OFF", driverIndex);
  RUNNING_ANIMATION = 0;
}

void Animation1(int endDriver, int delays){
  RUNNING_ANIMATION = 0; // Reset running animation
}
void Animation2(int endDriver, int delays){
}
void Animation3(int endDriver, int delays){
  RUNNING_ANIMATION = 0; // Reset running animation
}

void Animation4(int endDriver, int delays)
// This animation fades each color channel in and out sequentially
{
    
  for (int c = 0; c < 3; c++) // 0: r, 1: g, 2: b
  {
    // Fade out (255 -> 0) for each color channel
    for (int brightness = 255; brightness >= 0; brightness--)
    {
      CHECK_INTERRUPT();
      for (int i = 0; i < endDriver; i++)
      {
        if (c == 0) Drivers[i].r = brightness;
        else if (c == 1) Drivers[i].g = brightness;
        else Drivers[i].b = brightness;
      }
      FastLED.show();
      delay(delays);
    }
  }

  // Fade in (0 -> 255) for each color channel
  for (int c = 0; c < 3; c++) // 0: r, 1: g, 2: b
  {
    for (int brightness = 0; brightness <= 255; brightness++)
    {
      CHECK_INTERRUPT();
      for (int i = 0; i < endDriver; i++)
      {
        if (c == 0) Drivers[i].r = brightness;
        else if (c == 1) Drivers[i].g = brightness;
        else Drivers[i].b = brightness;
      }
      FastLED.show();
      delay(delays);
    }
  }
}

void Animation5(int endDriver, int delays)
// This animation crossfades between Red, Green, and Blue
{
  for (int brightness = 255; brightness >= 0; brightness--)
  {
    CHECK_INTERRUPT();
    for (int i = 0; i < endDriver; i++)
    {
      Drivers[i].r = brightness;
      if (Drivers[i].b != 255)
      {
        Drivers[i].b = 255 - brightness; // Fade out Blue
      }
    }
    FastLED.show();
    delay(delays);
  }

  for (int brightness = 255; brightness >= 0; brightness--)
  {
    CHECK_INTERRUPT();
    for (int i = 0; i < endDriver; i++)
    {
      Drivers[i].g = brightness;
      Drivers[i].r = 255 - brightness; // Fade out Red
    }
    FastLED.show();
    delay(delays);
  }

  for (int brightness = 255; brightness >= 0; brightness--)
  {
    CHECK_INTERRUPT();
    for (int i = 0; i < endDriver; i++)
    {
      Drivers[i].b = brightness;
      Drivers[i].g = 255 - brightness; // Fade out Green
    }
    FastLED.show();
    delay(delays);
  }
}

void Animation6(int endDriver, int delays)
{
    // ON all
  TurnONOFF(false, 0,endDriver);
  delay(1000);
  TurnONOFF(true, 0,endDriver);
  delay(1000);
  TurnONOFF(false, 0,endDriver);
  delay(1000);
  TurnONOFF(true, 0,endDriver);
  delay(1000);
  Animation4(endDriver, delays);
}

void animationHandler()
{
  if (!RUNNING_ANIMATION)
    return;

  int endDriver = BLE_ENABLED && ParsedBTCommand.hasArg2 ? ParsedBTCommand.arg2 : NUM_DRIVERS;
  int delays = BLE_ENABLED && ParsedBTCommand.hasArg3 ? ParsedBTCommand.arg3 : AnimationDelay;

  // int endDriver = NUM_DRIVERS;
  // int delays = AnimationDelay;

  // if (BUTTONS_ENABLED)
  // {
  //   endDriver = NUM_DRIVERS - NUM_AMENITIES_DRIVERS;
  // }
  // if (BLE_ENABLED)
  // {
  //   if (ParsedBTCommand.hasArg2)
  //   {
  //     endDriver = ParsedBTCommand.arg2;
  //   }
  //   if (ParsedBTCommand.hasArg3)
  //   {
  //     delays = ParsedBTCommand.arg3;
  //     // AnimationDelay = delays;
  //   }
  // }

  DEBUG_SERIAL.printf("Running Animation %d on drivers 0 to %d with %d delay\n", RUNNING_ANIMATION, endDriver, delays);
  switch (RUNNING_ANIMATION)
  {
  case 1:
    Animation1(endDriver, delays);
    break;
  case 2:
    Animation2(endDriver, delays);
    break;
  case 3:
    Animation3(endDriver, delays);
    break;
  case 4:
    Animation4(endDriver, delays);
    break;
  case 5:
    Animation5(endDriver, delays);
    break;
  case 6:
    Animation6(endDriver, delays);
    break;  
  default:
    break;
  }
}