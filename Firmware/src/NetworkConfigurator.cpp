#include "NetworkConfigurator.hpp"
#include <esp_log.h>
#include <esp_netif.h>
#include <lwip/inet.h>
#include <Webserver.hpp>
#include <Configuration.hpp>

static const char* TAG = "Network";

const IPAddress NetworkConfigurator::apIP_{192, 168, 4, 1};
const IPAddress NetworkConfigurator::apNetMask_{255, 255, 255, 0};
TaskHandle_t NetworkConfigurator::networkTask_ = nullptr;
DNSServer NetworkConfigurator::dnsServer_{};

void NetworkConfigurator::networkEvent(arduino_event_id_t event)
{
    switch(event){
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            ESP_LOGI(TAG, "Got station IP : %s (MAC : %s)", WiFi.localIP().toString().c_str(), WiFi.macAddress().c_str());
            break;
    }
}

bool NetworkConfigurator::startNetwork()
{
    /*
        Turn of warnings from HTTP server as redirecting traffic will yield
        lots of invalid requests
    */
    esp_log_level_set("httpd_uri", ESP_LOG_ERROR);
    esp_log_level_set("httpd_txrx", ESP_LOG_ERROR);
    esp_log_level_set("httpd_parse", ESP_LOG_ERROR);

    ESP_LOGI(TAG, "Starting network");
    WiFi.disconnect();
    WiFi.onEvent(networkEvent);

    WiFi.mode(WIFI_AP_STA); //Switch to AP_STA to be able to connect to a network + serving pages

    bool res = configureSoftAP();
    res |= configureStation();
    if(res){
        ESP_LOGI(TAG, "Network ready");
        //Adds DHCP option 114 to set our captive portal URI
        dhcp_set_captiveportal_url();
        //Starts a DNS server to redirect our pages
        if(xTaskCreate(networkTask, "NetManager", 4*1024, nullptr, 5, &networkTask_) != pdPASS){
            ESP_LOGE(TAG, "Unable to create DNS task!");
        }
        ESP_LOGI(TAG, "Starting web server");
        webServer.start();
    }

    //Register call-back to be notified if config changed
    configuration.registerListener(configurationChanged);

    return true;
}

void NetworkConfigurator::networkTask(void* args)
{
    dnsServer_.setErrorReplyCode(DNSReplyCode::NoError);
    uint16_t dnsPort = 53;
    //TODO: DNS should listen only on AP IP, not on the STA one
    dnsServer_.start(dnsPort, "*", apIP_);
    ESP_LOGI(TAG, "DNS server started!");
    while(true){
        //TODO: As for SNMP, maybe we can re-write it in a blocking manner?
        dnsServer_.processNextRequest();
        delay(10);
    }
}

void NetworkConfigurator::dhcp_set_captiveportal_url(void) {
    // get the IP of the access point to redirect to
    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_AP_DEF"), &ip_info);

    char ip_addr[16];
    inet_ntoa_r(ip_info.ip.addr, ip_addr, 16);
    ESP_LOGI(TAG, "Set up softAP with IP: %s", ip_addr);

    // turn the IP into a URI
    char* captiveportal_uri = (char*) malloc(32 * sizeof(char));
    assert(captiveportal_uri && "Failed to allocate captiveportal_uri");
    strcpy(captiveportal_uri, "http://");
    strcat(captiveportal_uri, ip_addr);

    // get a handle to configure DHCP with
    esp_netif_t* netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");

    // set the DHCP option 114
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_dhcps_stop(netif));
    ESP_ERROR_CHECK(esp_netif_dhcps_option(netif, ESP_NETIF_OP_SET, ESP_NETIF_CAPTIVEPORTAL_URI, captiveportal_uri, strlen(captiveportal_uri)));
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_dhcps_start(netif));
}

void NetworkConfigurator::configurationChanged(DeviceConfiguration::Parameter param)
{
    ESP_LOGI(TAG, "Configuration changed");
    switch(param){
        case DeviceConfiguration::Parameter::AP_SSID:
            configureSoftAP();
            break;
        case DeviceConfiguration::Parameter::STA_SSID:
        case DeviceConfiguration::Parameter::STA_IP:
            configureStation();
            break;
    }
}

bool NetworkConfigurator::configureSoftAP()
{
    bool res = true;
    ESP_LOGI(TAG, "Setup soft-AP");
    WiFi.softAPdisconnect();
    std::string apSSID, apPass;
    configuration.getAPSSID(apSSID, apPass);
    if(!apSSID.empty()){
        WiFi.softAPConfig(apIP_, apIP_, apNetMask_, (uint32_t)0, apIP_);
        ESP_LOGI(TAG, "AP : %s (%s)", apSSID.c_str(), apPass.c_str());
        const char* ssid = apSSID.c_str();
        const char* pass = apPass.empty() ? nullptr : apPass.c_str();
        res = WiFi.softAP(ssid, pass);
    }
    return res;
}

bool NetworkConfigurator::configureStation()
{
    bool res = true;
    ESP_LOGI(TAG, "Setup station");
    WiFi.disconnect();
    std::string apSSID, apPass;
    configuration.getStationSSID(apSSID, apPass);
    if(!apSSID.empty()){
        IPAddress ip, mask, gw;
        configuration.getStationIPConfiguration(ip, mask, gw);
        if(ip.operator uint32_t() == 0){
            ESP_LOGI(TAG, "Station using DHCP");
            WiFi.config(ip);
        }else{
            ESP_LOGI(TAG, "Station with static IP (%s)", ip.toString().c_str());
            WiFi.config(ip, mask, gw);
        }
        ESP_LOGI(TAG, "AP : %s (%s)", apSSID.c_str(), apPass.c_str());
        const char* ssid = apSSID.c_str();
        const char* pass = apPass.empty() ? nullptr : apPass.c_str();
        WiFi.begin(ssid, pass);
    }
    return res;
}
