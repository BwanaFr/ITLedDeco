#pragma once

#include <FreeRTOS.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <Configuration.hpp>

class NetworkConfigurator
{
public:
    static bool startNetwork();
    static void networkTask(void* args);

private:
    static void configurationChanged(DeviceConfiguration::Parameter param);
    static bool configureSoftAP();
    static bool configureStation();
    NetworkConfigurator() = delete;
    ~NetworkConfigurator() = delete;
    static void dhcp_set_captiveportal_url(void);
    static void networkEvent(arduino_event_id_t event);

    static const IPAddress apIP_;
    static const IPAddress apNetMask_;
    static TaskHandle_t networkTask_;
    static DNSServer dnsServer_;
    
};