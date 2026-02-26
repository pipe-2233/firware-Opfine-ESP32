#ifndef BASE_COMM_H
#define BASE_COMM_H

#include <Arduino.h>

class BaseComm {
public:
    BaseComm() : connected(false) {}
    virtual ~BaseComm() {}
    
    // Virtual interface
    virtual bool connect() = 0;
    virtual bool disconnect() = 0;
    virtual bool send(const String& topic, const String& payload) = 0;
    virtual void loop() = 0;
    virtual String getType() = 0;
    
    // Public API
    bool isConnected() const { return connected; }
    
protected:
    bool connected;
};

#endif
