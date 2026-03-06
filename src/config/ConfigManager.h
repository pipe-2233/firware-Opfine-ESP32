#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <vector>

struct SystemConfig {
    String device_name;
    uint8_t core0_frequency;
    uint8_t core1_frequency;
};

struct SensorConfig {
    String id;
    String type;  // "digital", "analog", "i2c", "uart", "spi"
    uint8_t pin;
    String mode;  // "input", "input_pullup", "input_pulldown"
    uint16_t sample_rate;
    bool enabled;
    
    // I2C specific
    uint8_t i2c_address;
    
    // UART specific
    uint8_t uart_rx;
    uint8_t uart_tx;
    uint32_t uart_baudrate;
    
    // Additional parameters (JSON string for flexibility)
    String extra_params;
};

struct MQTTConfig {
    String broker;
    uint16_t port;
    String user;
    String password;
    String topic_prefix;
    String client_id;
};

struct TCPConfig {
    String server;
    uint16_t port;
    bool use_ssl;
};

struct HTTPConfig {
    String endpoint;
    String method;
    String auth_token;
};

struct CommunicationConfig {
    String protocol;  // "mqtt", "tcp", "http", "websocket"
    MQTTConfig mqtt;
    TCPConfig tcp;
    HTTPConfig http;
};

struct WiFiConfig {
    String ssid;
    String password;
    bool ap_mode;
    String ap_ssid;
    String ap_password;
};

class ConfigManager {
public:
    ConfigManager();
    
    bool begin();
    bool loadConfig();
    bool saveConfig();
    bool loadFromJson(const String& jsonStr);
    String toJson();
    
    // Getters
    SystemConfig getSystemConfig() { return systemConfig; }
    std::vector<SensorConfig> getSensors() { return sensors; }
    CommunicationConfig getCommConfig() { return commConfig; }
    WiFiConfig getWiFiConfig() { return wifiConfig; }
    
    // Setters
    void setSystemConfig(const SystemConfig& config) { systemConfig = config; }
    void addSensor(const SensorConfig& sensor) { sensors.push_back(sensor); }
    void setCommConfig(const CommunicationConfig& config) { commConfig = config; }
    void setWiFiConfig(const WiFiConfig& config) { wifiConfig = config; }
    
    // Utilities
    void clearSensors() { sensors.clear(); }
    SensorConfig* getSensorById(const String& id);
    void removeSensor(const String& id);
    
private:
    SystemConfig systemConfig;
    std::vector<SensorConfig> sensors;
    CommunicationConfig commConfig;
    WiFiConfig wifiConfig;
    
    const char* configFile = "/config.json";
    
    bool parseJson(const JsonDocument& doc);
    void createDefaultConfig();
};

#endif
