#include <Configuration.hpp>
#include "esp_log.h"
#include <LittleFS.h>

#include <ETH.h>
#include "mbedtls/aes.h"
#include "mbedtls/cipher.h"

#define DEFAULT_AP_SSID "IT_Leds"
#define DEFAULT_AP_PASS "12345678"

#define DEFAULT_STA_SSID ""
#define DEFAULT_STA_PASS ""
#define DEFAULT_STA_IP "0.0.0.0"
#define DEFAULT_STA_SUBNET "0.0.0.0"
#define DEFAULT_STA_GATEWAY "0.0.0.0"

#define FLASH_SAVE_DELAY 2000
#define CFG_FILENAME "/config.json"

static const char* TAG = "Configuration";

DeviceConfiguration configuration;

DeviceConfiguration::DeviceConfiguration() :
        lastChange_{0}, mutexData_{nullptr}, mutexListeners_{nullptr},
        apSSID_{DEFAULT_AP_SSID}, apPass_{DEFAULT_AP_PASS},
        staSSID_{DEFAULT_STA_SSID}, staPass_{DEFAULT_STA_PASS},
        staIP_{DEFAULT_STA_IP}, staSubnet_{DEFAULT_STA_SUBNET},
        staGateway_{DEFAULT_STA_GATEWAY},
        configTask_(nullptr)
{
    mutexData_ = xSemaphoreCreateMutex();
    if(mutexData_ == NULL){
        ESP_LOGE(TAG, "Unable to create data mutex");
    }

    mutexListeners_ = xSemaphoreCreateMutex();
    if(mutexListeners_ == NULL){
        ESP_LOGE(TAG, "Unable to create listeners mutex");
    }
}

DeviceConfiguration::~DeviceConfiguration()
{
    if(mutexData_){
        vSemaphoreDelete(mutexData_);
        mutexData_ = nullptr;
    }
    if(mutexListeners_){
        vSemaphoreDelete(mutexListeners_);
        mutexListeners_ = nullptr;
    }
}

void DeviceConfiguration::begin()
{
    LittleFS.begin(true);
    if(configTask_ == NULL){
        xTaskCreate(configTask, "configTask", 4096, (void*)this, tskIDLE_PRIORITY + 1, &configTask_);
    }
    load();
}

void DeviceConfiguration::configTask(void* param)
{
    DeviceConfiguration* config = static_cast<DeviceConfiguration*>(param);
    vTaskSuspend(NULL);
    while(true){
        config->loop();
    }
}

void DeviceConfiguration::load()
{
    File configFile = LittleFS.open(CFG_FILENAME, "r");
    if(configFile){
        JsonDocument json;
        if(deserializeJson(json, configFile) == DeserializationError::Ok) {
            bool valid, changed;
            fromJSON(json, changed, valid, true, true);            
            if(xSemaphoreTake(mutexData_, portMAX_DELAY ) == pdTRUE)
            {
                lastChange_ = 0;        //Don't write to flash
                xSemaphoreGive(mutexData_);
            }
        }else{
            ESP_LOGE(TAG, "Unable to parse configuration file");
        }
        configFile.close();
    }else{
        ESP_LOGI(TAG, "Unable to load configuration file, using default");
        //Send default settings, no valid JSON file found (empty flash)
        setAPSSID(DEFAULT_AP_SSID, DEFAULT_AP_PASS, true);
        setStationSSID(DEFAULT_STA_SSID, DEFAULT_STA_PASS, true);
        setStationIPConfiguration(DEFAULT_STA_IP, DEFAULT_STA_SUBNET, DEFAULT_STA_GATEWAY, true);
    }
}

void DeviceConfiguration::fromJSON(const JsonDocument& doc, bool& changed, bool& valid, bool cipheredSecrets, bool forceNotification)
{
    valid = false;
    changed = false;

    //WiFi AP SSID info, must be sent by pair
    if(doc["apSSID"].is<std::string>()){
        std::string pass = apPass_;
        if(doc["apPass"].is<std::string>()){
            pass = doc["apPass"].as<std::string>();
            if(cipheredSecrets){
                pass = decrypt(pass);
            }
        }
        changed |= setAPSSID(doc["apSSID"].as<std::string>(), pass, forceNotification);
        valid = true;
    }

    //WiFi STA SSID info
    if(doc["staSSID"].is<std::string>()){
        std::string pass = staPass_;
        if(doc["staPass"].is<std::string>()){
            pass = doc["staPass"].as<std::string>();
            if(cipheredSecrets){
                pass = decrypt(pass);
            }
        }
        changed |= setStationSSID(doc["staSSID"].as<std::string>(), pass, forceNotification);
        valid = true;
    }
    
    //WiFi STA
    if(doc["staIP"].is<std::string>()){
        IPAddress staIP;
        staIP.fromString(doc["staIP"].as<std::string>().c_str());
        
        IPAddress staSub(staSubnet_);
        if(doc["staSubnet"].is<std::string>()){
            staSub.fromString(doc["staSubnet"].as<std::string>().c_str());
        }

        IPAddress staGw(staGateway_);
        if(doc["staGateway"].is<std::string>()){
            staGw.fromString(doc["staGateway"].as<std::string>().c_str());
        }
        changed |= setStationIPConfiguration(staIP, staSub, staGw, forceNotification);
        valid = true;
    }
}

bool DeviceConfiguration::setAPSSID(const std::string& ssid, const std::string& pass, bool forceNotification)
{
    bool changed = false;
    if(xSemaphoreTake(mutexData_, portMAX_DELAY ) == pdTRUE)
    {
        if((apSSID_ != ssid) || (apPass_ != pass))
        {
            apSSID_ = ssid;
            apPass_ = pass;
            configurationChanged();
            changed = true;
        }
        xSemaphoreGive(mutexData_);
        if(changed || forceNotification){
            notifyListeners(Parameter::AP_SSID);
        }
    }
    return changed;
}

void DeviceConfiguration::getAPSSID(std::string& ssid, std::string& pass)
{
    if(xSemaphoreTake(mutexData_, portMAX_DELAY ) == pdTRUE)
    {
        ssid = apSSID_;
        pass = apPass_;
        xSemaphoreGive(mutexData_);
    }
}

bool DeviceConfiguration::setStationSSID(const std::string& ssid, const std::string& pass, bool forceNotification)
{
    bool changed = false;
    if(xSemaphoreTake(mutexData_, portMAX_DELAY ) == pdTRUE)
    {
        if((staSSID_ != ssid) || (staPass_ != pass))
        {
            staSSID_ = ssid;
            staPass_ = pass;
            configurationChanged();
            changed = true;
        }
        xSemaphoreGive(mutexData_);
        if(changed || forceNotification){
            notifyListeners(Parameter::STA_SSID);
        }
    }
    return changed;
}

void DeviceConfiguration::getStationSSID(std::string& ssid, std::string& pass)
{
    if(xSemaphoreTake(mutexData_, portMAX_DELAY ) == pdTRUE)
    {
        ssid = staSSID_;
        pass = staPass_;
        xSemaphoreGive(mutexData_);
    }
}

bool DeviceConfiguration::setStationIPConfiguration(const IPAddress& ip, const IPAddress& subnet, const IPAddress& gateway, bool forceNotification)
{
    bool changed = false;
    if(xSemaphoreTake(mutexData_, portMAX_DELAY ) == pdTRUE)
    {
        if((staIP_ != ip) || (staSubnet_ != subnet) || (staGateway_ != gateway))
        {
            staIP_ = ip;
            staSubnet_ = subnet;
            staGateway_ = gateway;
            configurationChanged();
            changed = true;
        }
        xSemaphoreGive(mutexData_);
        if(changed || forceNotification){
            notifyListeners(Parameter::STA_IP);
        }
    }
    return changed;
}

void DeviceConfiguration::getStationIPConfiguration(IPAddress& ip, IPAddress& subnet, IPAddress& gateway)
{
    if(xSemaphoreTake(mutexData_, portMAX_DELAY ) == pdTRUE)
    {
        ip = staIP_;
        subnet = staSubnet_;
        gateway = staGateway_;
        xSemaphoreGive(mutexData_);
    }
}

/**
 * Loop to delay flash writing
 */
void DeviceConfiguration::loop()
{
    vTaskDelay(pdMS_TO_TICKS(FLASH_SAVE_DELAY));
    bool doWrite = false;
    if(xSemaphoreTake(mutexData_, 0) == pdTRUE)
    {
        if(lastChange_ != 0){
            //Something changed
            doWrite = true;
            lastChange_ = 0;
        }
        xSemaphoreGive(mutexData_);
    }

    if(doWrite){
        write();
    }

    bool suspend = false;
    if(xSemaphoreTake(mutexData_, 0) == pdTRUE)
    {
        suspend = (lastChange_ == 0);
        xSemaphoreGive(mutexData_);
    }
    if(suspend){
        //Suspend this task
        vTaskSuspend(NULL);
    }
}

void DeviceConfiguration::configurationChanged()
{
    //Sets last change flag
    lastChange_ = millis();
    //Wakeup to flash write task
    vTaskResume(configTask_);
}

/**
 * Create a JSON version of the configuration
 */
void DeviceConfiguration::toJSONString(std::string& str)
{
    JsonDocument doc;
    toJSON(doc);
    serializeJson(doc, str);
}

void DeviceConfiguration::toJSON(JsonDocument& doc, bool includeSecrets)
{
    if(xSemaphoreTake(mutexData_, portMAX_DELAY ) == pdTRUE)
    {
        doc["apSSID"] = apSSID_;
        doc["staSSID"] = staSSID_;
        doc["staIP"] = staIP_.toString();
        doc["staSubnet"] = staSubnet_.toString();
        doc["staGateway"] = staGateway_.toString();

        if(includeSecrets){
            doc["apPass"] = encrypt(apPass_);
            doc["staPass"] = encrypt(staPass_);
        }else{
            doc["apPass"] = std::string(apPass_.size(), ' ');
            doc["staPass"] = std::string(staPass_.size(), ' ');
        }
        xSemaphoreGive(mutexData_);
    }
}

void DeviceConfiguration::registerListener(ParameterListener listener)
{
    if(xSemaphoreTake(mutexListeners_, portMAX_DELAY ) == pdTRUE)
    {
        listeners_.push_back(listener);
        xSemaphoreGive(mutexListeners_);
    }
}

void DeviceConfiguration::notifyListeners(Parameter changed)
{
    if(xSemaphoreTake(mutexListeners_, portMAX_DELAY ) == pdTRUE)
    {
        for(ParameterListener l: listeners_){
            l(changed);
        }
        xSemaphoreGive(mutexListeners_);
    }
}

void DeviceConfiguration::write()
{
    ESP_LOGI(TAG, "Saving configuration to flash");
    File cfgFile = LittleFS.open(CFG_FILENAME, "w");
    JsonDocument doc;
    toJSON(doc, true);
    serializeJson(doc, cfgFile);
    cfgFile.close();
    std::string str;
    serializeJson(doc, str);
    ESP_LOGI(TAG, "%s", str.c_str());
}

void DeviceConfiguration::generateKeys(unsigned char iv[16], unsigned char key[128])
{
    uint8_t mac[6] = {0, 0, 0, 0, 0, 0};
    ETH.macAddress(mac);
    for(int i=0;i<16;++i){
        iv[i] = mac[i%6];
    }
    for(int i=0;i<128;++i){
        key[i] = mac[i%6] + (mac[0] << 3);
    }
}

std::string DeviceConfiguration::encrypt(const std::string& input)
{
    unsigned char* out;
    std::string inData = input;
    unsigned long cipherLen = inData.size() + 16 - (inData.size() % 16);
    inData.resize(cipherLen);
    out = (unsigned char*)malloc(cipherLen);
    if(!out){
        ESP_LOGE(TAG, "Unable to allocate crypto buffer!");
    }
    unsigned char iv[16];
    unsigned char key[128];
    generateKeys(iv, key);
	mbedtls_aes_context aes;
	mbedtls_aes_init(&aes);
	mbedtls_aes_setkey_enc(&aes, key, 128);
	mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_ENCRYPT, cipherLen, iv, (const unsigned char*)input.c_str(), out);
	mbedtls_aes_free(&aes);

    std::string ret;
    ret.resize(cipherLen*2);
    for(int i=0;i<cipherLen;++i){
        sprintf(&ret[i*2], "%02X", out[i]);
    }
    return ret;
}

std::string DeviceConfiguration::decrypt(const std::string& input)
{
    std::string ret;
    size_t dataSize = input.length()/2;
    ret.resize(dataSize);
    unsigned char* in;
    unsigned char iv[16];
    unsigned char key[128];
    generateKeys(iv, key);

    in = (unsigned char*)malloc(dataSize);
    if(!in){
        ESP_LOGE(TAG, "Unable to allocate crypto buffer! (%u bytes)", dataSize);
    }else{
        for(int i=0;i<dataSize;++i){
            char data[3] = {input[i*2], input[i*2+1], '\0'};
            sscanf(data, "%02X", &in[i]);
        }
    }
    mbedtls_aes_context aes;
	mbedtls_aes_init(&aes);
	mbedtls_aes_setkey_enc(&aes, key, 128);
	mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_DECRYPT, dataSize, iv, in, (unsigned char*)&ret[0]);
	mbedtls_aes_free(&aes);
    ret.resize(strlen(ret.c_str()));
    return ret;
}
