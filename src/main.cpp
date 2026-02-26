#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include "config/ConfigManager.h"
#include "core/TaskManager.h"
#include "web/WebServer.h"

// Instancias globales
ConfigManager configManager;
TaskManager taskManager;
ConfigWebServer* webServer = nullptr;

// Variables WiFi
bool wifiConnected = false;

void setupWiFi();
void printSystemInfo();

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n\n=================================");
    Serial.println("ESP32 Modular Firmware v1.0");
    Serial.println("=================================\n");
    
    // I2C bus initialization (SDA=21, SCL=22)
    Wire.begin(21, 22);
    
    // Load system configuration from SPIFFS
    Serial.println("1. Cargando configuración...");
    if (!configManager.begin()) {
        Serial.println("Error al cargar configuración. Usando configuración por defecto.");
    }
    
    // WiFi setup
    Serial.println("\n2. Configurando WiFi...");
    setupWiFi();
    
    // HTTP configuration server (port 80)
    Serial.println("\n3. Iniciando servidor web...");
    webServer = new ConfigWebServer(configManager);
    webServer->begin();
    
    // FreeRTOS task manager initialization
    Serial.println("\n4. Inicializando gestor de tareas...");
    if (!taskManager.begin()) {
        Serial.println("Error inicializando TaskManager");
        return;
    }
    
    // Iniciar tareas en ambos cores
    Serial.println("\n5. Iniciando tareas dual-core...");
    taskManager.startTasks();
    
    Serial.println("\n=================================");
    Serial.println("Sistema Iniciado Exitosamente");
    Serial.println("=================================");
    
    printSystemInfo();
}

void loop() {
    // Mantener el servidor web activo
    if (webServer != nullptr) {
        webServer->loop();
    }
    
    // WiFi reconnection handler
    if (WiFi.status() != WL_CONNECTED && wifiConnected) {
        Serial.println("[WiFi] Conexión perdida. Intentando reconectar...");
        wifiConnected = false;
        setupWiFi();
    }
    
    delay(10);
}

void setupWiFi() {
    WiFiConfig wifiConfig = configManager.getWiFiConfig();
    
    if (wifiConfig.ap_mode) {
        // Start Access Point for configuration interface
        Serial.println("Iniciando modo Access Point...");
        Serial.printf("  SSID: %s\n", wifiConfig.ap_ssid.c_str());
        Serial.printf("  Password: %s\n", wifiConfig.ap_password.c_str());
        
        WiFi.mode(WIFI_AP);
        WiFi.softAP(wifiConfig.ap_ssid.c_str(), wifiConfig.ap_password.c_str());
        
        IPAddress IP = WiFi.softAPIP();
        Serial.print("IP del AP: ");
        Serial.println(IP);
        Serial.println("Conéctate al AP y abre http://192.168.4.1");
        
        wifiConnected = true;
    } else {
        // Modo cliente WiFi
        if (wifiConfig.ssid.length() == 0) {
            Serial.println("SSID no configurado. Activando modo AP...");
            wifiConfig.ap_mode = true;
            configManager.setWiFiConfig(wifiConfig);
            setupWiFi();
            return;
        }
        
        Serial.println("Conectando a WiFi...");
        Serial.printf("  SSID: %s\n", wifiConfig.ssid.c_str());
        
        WiFi.mode(WIFI_STA);
        WiFi.begin(wifiConfig.ssid.c_str(), wifiConfig.password.c_str());
        
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            delay(500);
            Serial.print(".");
            attempts++;
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\nConectado a WiFi!");
            Serial.print("IP: ");
            Serial.println(WiFi.localIP());
            wifiConnected = true;
        } else {
            Serial.println("\nNo se pudo conectar. Activando modo AP...");
            wifiConfig.ap_mode = true;
            configManager.setWiFiConfig(wifiConfig);
            setupWiFi();
        }
    }
}

void printSystemInfo() {
    Serial.println("\n--- Información del Sistema ---");
    
    SystemConfig sysConfig = configManager.getSystemConfig();
    Serial.printf("Nombre del dispositivo: %s\n", sysConfig.device_name.c_str());
    Serial.printf("Frecuencia Core 0: %d MHz\n", sysConfig.core0_frequency);
    Serial.printf("Frecuencia Core 1: %d MHz\n", sysConfig.core1_frequency);
    
    Serial.println("\n--- Sensores Configurados ---");
    auto sensors = configManager.getSensors();
    if (sensors.empty()) {
        Serial.println("No hay sensores configurados");
    } else {
        for (const auto& sensor : sensors) {
            Serial.printf("  [%s] Tipo: %s, Pin: %d, Estado: %s\n",
                         sensor.id.c_str(),
                         sensor.type.c_str(),
                         sensor.pin,
                         sensor.enabled ? "Habilitado" : "Deshabilitado");
        }
    }
    
    Serial.println("\n--- Comunicación ---");
    auto commConfig = configManager.getCommConfig();
    Serial.printf("Protocolo: %s\n", commConfig.protocol.c_str());
    
    if (commConfig.protocol == "mqtt") {
        Serial.printf("  Broker: %s:%d\n", commConfig.mqtt.broker.c_str(), commConfig.mqtt.port);
    } else if (commConfig.protocol == "tcp") {
        Serial.printf("  Servidor: %s:%d\n", commConfig.tcp.server.c_str(), commConfig.tcp.port);
    } else if (commConfig.protocol == "http") {
        Serial.printf("  Endpoint: %s\n", commConfig.http.endpoint.c_str());
    }
    
    Serial.println("\n--- Memoria ---");
    Serial.printf("Heap libre: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("PSRAM: %s\n", psramFound() ? "Disponible" : "No disponible");
    
    Serial.println("\n-------------------------------\n");
}
