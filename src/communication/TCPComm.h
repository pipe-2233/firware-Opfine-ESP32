#ifndef TCP_COMM_H
#define TCP_COMM_H

#include "BaseComm.h"
#include <WiFi.h>

struct TCPCredentials {
    String server;
    uint16_t port;
    bool useSSL;
};

class TCPComm : public BaseComm {
public:
    TCPComm(const TCPCredentials& credentials);
    ~TCPComm();
    
    bool connect() override;
    bool disconnect() override;
    bool send(const String& topic, const String& payload) override;
    void loop() override;
    String getType() override { return "tcp"; }
    
    // TCP raw operations
    bool sendRaw(const uint8_t* data, size_t length);
    int available();
    String receive();
    
private:
    TCPCredentials credentials;
    WiFiClient* client;
    
    unsigned long lastReconnectAttempt;
    uint16_t reconnectDelay;
    
    bool reconnect();
};

#endif
