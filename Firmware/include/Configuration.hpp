#ifndef _CONFIGURATION_HPP__
#define _CONFIGURATION_HPP__

#include <string>
#include <IPAddress.h>
#include <ArduinoJson.h>
#include <vector>
#include <functional>
#include <FreeRTOS.h>

class DeviceConfiguration {
public:
    /**
     * Enumeration of parameter
     */
    enum class Parameter {
        AP_SSID = 0,   //WiFi AP mode configuration
        STA_SSID,      //WiFi station configuration
        STA_IP,        //WiFi station IP parameters
//TODO: Implement more ?
    };

    DeviceConfiguration();
    virtual ~DeviceConfiguration();

    /**
     * Setup
     */
    void begin();

    /**
     * Loads configuration from JSON file in flash
     */
    void load();

    /**
     * Sets access point SSID
     * @param ssid If empty, no AP will be created
     * @param pass Wifi passphrase, open network if empty
     * @param forceNotification True to notifiy new configuration (even if not changed)
     * @return true if changed
     */
    bool setAPSSID(const std::string& ssid, const std::string& pass, bool forceNotification = false);

    /**
     * Gets AP SSID
     * @param ssid Station SSID, if empty no AP is used
     * @param pass Wifi passphrase, open network if empty
     */
    void getAPSSID(std::string& ssid, std::string& pass);

    /**
     * Sets station SSID
     * @param ssid If empty, no station will be connected
     * @param pass Wifi passphrase, open network if empty
     * @param forceNotification True to notifiy new configuration (even if not changed)
     * @return true if changed
     */
    bool setStationSSID(const std::string& ssid, const std::string& pass, bool forceNotification = false);

    /**
     * Gets station SSID
     * @param ssid Station SSID, if empty no station is used
     * @param pass Wifi passphrase, open network if empty
     */
    void getStationSSID(std::string& ssid, std::string& pass);

    /**
     * Sets station IP address configuration
     * @param ip IP Address of this device (0.0.0.0 to use DHCP)
     * @param subnet IP subnet mask
     * @param gateway First gateway IP
     * @param forceNotification True to notifiy new configuration (even if not changed)
     * @return true if changed
     */
    bool setStationIPConfiguration(const IPAddress& ip, const IPAddress& subnet, const IPAddress& gateway, bool forceNotification = false);

    /**
     * Gets station IP address of this device
     * @param ip IP address of this device (0.0.0.0 to use DHCP)
     * @param subnet Subnet mask
     * @param gateway First gateway IP
     */
    void getStationIPConfiguration(IPAddress& ip, IPAddress& subnet, IPAddress& gateway);

    /**
     * Create a JSON string of the configuration
     */
    void toJSONString(std::string& str);

    /**
     * Fill configuration to JSON document
     * @param doc JSON document to fill
     * @param includeSecrets True to include ciphered passwords
     */
    void toJSON(JsonDocument& doc, bool includeSecrets = false);

    /**
     * Loads from JSON document
     * @param doc JSON document to load
     * @param changed True if the configuration changed
     * @param valid True if the configuration is valid
     * @param cipheredSecrets True if ciphered secrets maybe present in the JSON document
     * @param forceNotification True to notifiy new configuration (even if not changed)
     */
    void fromJSON(const JsonDocument& doc, bool& changed, bool& valid, bool cipheredSecrets, bool forceNotification = false);

    /**
     * Paramter change callback
     */
    typedef std::function<void(Parameter)> ParameterListener;

    /**
     * Register a new listener
     */
    void registerListener(ParameterListener listener);

    void saveFXConfiguration();
    void loadFXConfiguration(JsonDocument& doc);

private:
    unsigned long lastChange_;                  //!< Last change of one setting
    unsigned long lastFxChange_;                //!< Last change of one FX setting
    SemaphoreHandle_t mutexData_;               //!< Protect access to ressources
    SemaphoreHandle_t mutexListeners_;          //!< Protect access to listeners vector
    std::string apSSID_;                        //!< AP SSID
    std::string apPass_;                        //!< AP passphrase
    std::string staSSID_;                       //!< Station SSID
    std::string staPass_;                       //!< Station passphrase
    IPAddress staIP_;                           //!< Station Device IP if static
    IPAddress staSubnet_;                       //!< Device Subnet if static IP
    IPAddress staGateway_;                      //!< Next gateway if static IP
    std::vector<ParameterListener> listeners_;  //!< Configuration listeners
    TaskHandle_t configTask_;                   //!<< Handle to the task

    void notifyListeners(Parameter changed);

    /**
     * Write to flash
     */
    void write();

    /**
     * Loop to delay flash writing
     */
    void loop();

    /**
     * Helper to set configuration changed
     */
    void configurationChanged();

    static std::string encrypt(const std::string& input);
    static std::string decrypt(const std::string& input);
    static void generateKeys(unsigned char iv[16], unsigned char key[128]);
    static void configTask(void* param);
    void saveFXConfiguration(const JsonDocument& doc);
};

extern DeviceConfiguration configuration;

#endif