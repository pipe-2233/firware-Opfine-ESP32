#ifndef MQTT_COMM_H
#define MQTT_COMM_H

#include "BaseComm.h"
#include <WiFi.h>
#include <PubSubClient.h>

struct MQTTCredentials {
    String broker;
    uint16_t port;
    String user;
    String password;
    String clientId;
    String topicPrefix;
};

class MQTTComm : public BaseComm {
public:
    MQTTComm(const MQTTCredentials& credentials);
    ~MQTTComm();
    
    bool connect() override;
    bool disconnect() override;
    bool send(const String& topic, const String& payload) override;
    void loop() override;
    String getType() override { return "mqtt"; }
    
    // MQTT-specific operations
    void setCallback(std::function<void(String, String)> callback);
    bool subscribe(const String& topic);
    bool unsubscribe(const String& topic);
    
private:
    MQTTCredentials credentials;
    WiFiClient wifiClient;
    PubSubClient* mqttClient;
    
    unsigned long lastReconnectAttempt;
    uint16_t reconnectDelay;
    
    std::function<void(String, String)> messageCallback;
    
    bool reconnect();
    static void mqttCallback(char* topic, byte* payload, unsigned int length);
    static MQTTComm* instance;  // Para callback estático
};

#endif
