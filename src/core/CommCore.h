#ifndef COMM_CORE_H
#define COMM_CORE_H

#include <Arduino.h>
#include "../config/ConfigManager.h"
#include "../communication/BaseComm.h"
#include "TaskManager.h"

class CommCore {
public:
    CommCore();
    ~CommCore();
    
    bool begin();
    void loop();
    void stop();
    
    void loadCommunication(ConfigManager& config);
    void setCommProtocol(BaseComm* comm);
    
    bool sendData(const String& topic, const String& payload);
    
private:
    BaseComm* commProtocol;
    bool initialized;
    unsigned long lastConnectionCheck;
    
    void processIncomingMessages();
    void checkConnection();
};

#endif
