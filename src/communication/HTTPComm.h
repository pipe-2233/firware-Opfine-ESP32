#ifndef HTTP_COMM_H
#define HTTP_COMM_H

#include "BaseComm.h"
#include <WiFi.h>
#include <HTTPClient.h>

struct HTTPCredentials {
    String endpoint;
    String method;  // GET, POST, PUT
    String authToken;
};

class HTTPComm : public BaseComm {
public:
    HTTPComm(const HTTPCredentials& credentials);
    ~HTTPComm();
    
    bool connect() override;
    bool disconnect() override;
    bool send(const String& topic, const String& payload) override;
    void loop() override;
    String getType() override { return "http"; }
    
    // HTTP operations
    bool sendJSON(const String& json);
    bool sendFormData(const String& data);
    void addHeader(const String& name, const String& value);
    
private:
    HTTPCredentials credentials;
    HTTPClient http;
    
    bool makeRequest(const String& url, const String& payload);
};

#endif
