#include "MQTTComm.h"

MQTTComm* MQTTComm::instance = nullptr;

MQTTComm::MQTTComm(const MQTTCredentials& creds) 
    : credentials(creds), lastReconnectAttempt(0), reconnectDelay(5000) {
    
    mqttClient = new PubSubClient(wifiClient);
    mqttClient->setServer(credentials.broker.c_str(), credentials.port);
    mqttClient->setCallback(mqttCallback);
    
    instance = this;
}

MQTTComm::~MQTTComm() {
    disconnect();
    delete mqttClient;
}

bool MQTTComm::connect() {
    Serial.println("[MQTT] Intentando conectar...");
    Serial.printf("  Broker: %s:%d\n", credentials.broker.c_str(), credentials.port);
    Serial.printf("  Client ID: %s\n", credentials.clientId.c_str());
    
    bool result;
    
    if (credentials.user.length() > 0) {
        result = mqttClient->connect(
            credentials.clientId.c_str(),
            credentials.user.c_str(),
            credentials.password.c_str()
        );
    } else {
        result = mqttClient->connect(credentials.clientId.c_str());
    }
    
    if (result) {
        Serial.println("[MQTT] Conectado exitosamente");
        connected = true;
        
        // Suscribirse a tópico de comandos
        String cmdTopic = credentials.topicPrefix + "commands/#";
        subscribe(cmdTopic);
    } else {
        Serial.printf("[MQTT] Error de conexión: %d\n", mqttClient->state());
        connected = false;
    }
    
    return result;
}

bool MQTTComm::disconnect() {
    if (mqttClient->connected()) {
        mqttClient->disconnect();
    }
    connected = false;
    Serial.println("[MQTT] Desconectado");
    return true;
}

bool MQTTComm::send(const String& topic, const String& payload) {
    if (!connected) {
        return false;
    }
    
    String fullTopic = credentials.topicPrefix + topic;
    bool result = mqttClient->publish(fullTopic.c_str(), payload.c_str());
    
    if (result) {
        Serial.printf("[MQTT] Publicado: %s -> %s\n", fullTopic.c_str(), payload.c_str());
    } else {
        Serial.printf("[MQTT] Error publicando en: %s\n", fullTopic.c_str());
    }
    
    return result;
}

void MQTTComm::loop() {
    if (!mqttClient->connected()) {
        unsigned long now = millis();
        if (now - lastReconnectAttempt > reconnectDelay) {
            lastReconnectAttempt = now;
            if (reconnect()) {
                lastReconnectAttempt = 0;
            }
        }
    } else {
        mqttClient->loop();
    }
}

bool MQTTComm::reconnect() {
    Serial.println("[MQTT] Intentando reconectar...");
    return connect();
}

void MQTTComm::setCallback(std::function<void(String, String)> callback) {
    messageCallback = callback;
}

bool MQTTComm::subscribe(const String& topic) {
    if (!connected) return false;
    
    bool result = mqttClient->subscribe(topic.c_str());
    if (result) {
        Serial.printf("[MQTT] Suscrito a: %s\n", topic.c_str());
    }
    return result;
}

bool MQTTComm::unsubscribe(const String& topic) {
    if (!connected) return false;
    return mqttClient->unsubscribe(topic.c_str());
}

void MQTTComm::mqttCallback(char* topic, byte* payload, unsigned int length) {
    if (instance == nullptr) return;
    
    String topicStr = String(topic);
    String payloadStr = "";
    
    for (unsigned int i = 0; i < length; i++) {
        payloadStr += (char)payload[i];
    }
    
    Serial.printf("[MQTT] Mensaje recibido: %s -> %s\n", topicStr.c_str(), payloadStr.c_str());
    
    if (instance->messageCallback) {
        instance->messageCallback(topicStr, payloadStr);
    }
}
