/**
 * IT speaker LED strip controller
 * Description : This firmware controls WS2812b from Aliexpress (aliexpress.com/item/1005007587121741.html)
 * The LED strip is arranged to form IT letters. Here is the LED strip layout
 *
 * ExxxxxxxxxxF       IxxxxxxxxxxJ
 *      D                  H
 *      x                  x
 *      x                  x
 *      C                  G
 * AxxxxxxxxxxB
 *
 * Where B-A == F-E == J-I  (called LETTER_WIDTH)
 * and D-C == H-G           (called LETTER_HEIGHT)
 *
 */

#include <WiFi.h>
#include <Arduino.h>
#include "esp_log.h"

#include "PCM1808.hpp"
#include "Webserver.hpp"


static const char* TAG = "Main";

PCM1808 pcm;
extern Webserver webServer;

void netEvent(arduino_event_id_t event)
{
    switch(event){
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            ESP_LOGI(TAG, "Got IP : %s", WiFi.localIP().toString().c_str());
            webServer.start();
            break;
    }
}

void setup()
{
    //Starts serial
    Serial.begin(115200);
    WiFi.onEvent(netEvent);
    WiFi.begin("CERN");
    pcm.start();
}


void loop()
{
    pcm.read();
    delay(1);
}
