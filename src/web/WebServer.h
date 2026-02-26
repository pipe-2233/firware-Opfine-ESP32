#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "../config/ConfigManager.h"

class ConfigWebServer {
public:
    ConfigWebServer(ConfigManager& config);
    
    bool begin();
    void loop();
    void stop();
    
    WebServer* getServer() { return server; }
    
private:
    ConfigManager& configManager;
    WebServer* server;
    
    // Handlers HTTP
    void handleRoot();
    void handleGetConfig();
    void handleSaveConfig();
    void handleSensors();
    void handleAddSensor();
    void handleRemoveSensor();
    void handleCommunication();
    void handleRestart();
    void handleNotFound();
    
    // Utilidades
    String getHTMLHeader();
    String getHTMLFooter();
    String generateConfigPage();
    String generateSensorsPage();
    String generateCommPage();
};

#endif
