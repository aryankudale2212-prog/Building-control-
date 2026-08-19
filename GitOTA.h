#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include <ArduinoOTA.h>

// function takes parameters wifi_ssid and wifi_pass
bool connectToWiFi(std::string wifi_ssid, std::string wifi_pass)
{
  if (WiFi.status() == WL_CONNECTED)
  {
    // return true; // Already connected
    WiFi.disconnect(); // Disconnect before reconnecting
    delay(1000);       // Short delay to ensure disconnection
  }
  DEBUG_SERIAL.printf("Connecting to WiFi SSID: %s\n", wifi_ssid.c_str());
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid.c_str(), wifi_pass.c_str());
  uint8_t attempts = 10;
  while (--attempts && WiFi.status() != WL_CONNECTED)
  {
    DEBUG_SERIAL.print(".");
    delay(500);
  }
  return WiFi.status() == WL_CONNECTED;
}

bool startOTAUpdate(WiFiClient *client, int contentLength)
{
  DEBUG_SERIAL.println("Initializing update...");
  if (!Update.begin(contentLength))
  {
    DEBUG_SERIAL.printf("Update begin failed: %s\n", Update.errorString());
    return false;
  }

  DEBUG_SERIAL.println("Writing firmware...");
  size_t written = 0;
  int progress = 0;
  int lastProgress = 0;

  // Timeout variables
  const unsigned long timeoutDuration = 120 * 1000; // 10 seconds timeout
  unsigned long lastDataTime = millis();

  while (written < contentLength)
  {
    if (client->available())
    {
      uint8_t buffer[128];
      size_t len = client->read(buffer, sizeof(buffer));
      if (len > 0)
      {
        Update.write(buffer, len);
        written += len;

        // Calculate and print progress
        progress = (written * 100) / contentLength;
        if (progress != lastProgress)
        {
          DEBUG_SERIAL.printf("Writing Progress: %d%%\n", progress);
          lastProgress = progress;
        }
      }
    }
    // Check for timeout
    if (millis() - lastDataTime > timeoutDuration)
    {
      DEBUG_SERIAL.println("Timeout: No data received for too long. Aborting update...");
      Update.abort();
      return false;
    }

    yield();
  }
  DEBUG_SERIAL.println("\nWriting complete");

  if (written != contentLength)
  {
    DEBUG_SERIAL.printf("Error: Write incomplete. Expected %d but got %d bytes\n", contentLength, written);
    Update.abort();
    return false;
  }

  if (!Update.end())
  {
    DEBUG_SERIAL.printf("Error: Update end failed: %s\n", Update.errorString());
    return false;
  }

  DEBUG_SERIAL.println("Update successfully completed");
  return true;
}

void checkForFirmwareUpdate()
{
  DEBUG_SERIAL.println("Checking for firmware update...");
  if (WiFi.status() != WL_CONNECTED)
  {
    DEBUG_SERIAL.println("WiFi not connected");
    return;
  }

  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.begin(FIRMWARE_URL);

  int httpCode = http.GET();
  DEBUG_SERIAL.printf("HTTP GET code: %d\n", httpCode);

  if (httpCode == HTTP_CODE_OK)
  {
    int contentLength = http.getSize();
    DEBUG_SERIAL.printf("Firmware size: %d bytes\n", contentLength);

    if (contentLength > 0)
    {
      WiFiClient *stream = http.getStreamPtr();
      if (startOTAUpdate(stream, contentLength))
      {
        DEBUG_SERIAL.println("OTA update successful, restarting...");
        CurrentOTAStatus = OTACompleted; // OTA Completed
        delay(1000);
        ESP.restart();
      }
      else
      {
        DEBUG_SERIAL.println("OTA update failed");
        CurrentOTAStatus = OTAFailed; // OTA Failed
      }
    }
    else
    {
      DEBUG_SERIAL.println("Invalid firmware size");
      CurrentOTAStatus = OTAFailed; // OTA Failed
    }
  }
  else
  {
    DEBUG_SERIAL.printf("Failed to fetch firmware. HTTP code: %d\n", httpCode);
    CurrentOTAStatus = OTAFailed; // OTA Failed
  }
  http.end();
}

void checkOTA(bool force = false)
{
#ifndef OTA
  return;
#endif

  if (OTAInProgress == OTALocal)
  {
    ArduinoOTA.handle();
    return;
  }

  if (CurrentOTAStatus != OTAGit && !force)
    return;

  stopBLE();
  if (connectToWiFi(WIFI_SSID, WIFI_PASS))
  {
    DEBUG_SERIAL.println("Connected.\nStarting OTA...");
    checkForFirmwareUpdate();
  }
  else
  {
    DEBUG_SERIAL.println("Failed to connect to WiFi for OTA");
    CurrentOTAStatus = OTAFailed; // OTA Failed
  }
}

/*
#pragma once
#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>



bool startOTAUpdate(WiFiClient* client, size_t size) {
    if (!Update.begin(size)) return false;
    size_t written = Update.writeStream(*client);
    if (written != size) {
        Update.abort();
        return false;
    }
    return Update.end();
}

void checkForFirmwareUpdate() {
    if (WiFi.status() != WL_CONNECTED) return;

    HTTPClient http;
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.begin(FW_URL);

    if (http.GET() == HTTP_CODE_OK) {
        int size = http.getSize();
        if (size > 0 && startOTAUpdate(http.getStreamPtr(), size)) {
            ESP.restart();
        }
    }
    http.end();
}
*/
