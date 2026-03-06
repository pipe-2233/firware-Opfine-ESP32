#include "ConfigManager.h"

ConfigManager::ConfigManager() {
    // Default system configuration
    systemConfig.device_name = "ESP32-Node-01";
    systemConfig.core0_frequency = 240;
    systemConfig.core1_frequency = 240;
    
    wifiConfig.ap_mode = true;
    wifiConfig.ap_ssid = "ESP32-Config";
    wifiConfig.ap_password = "config123";
}

bool ConfigManager::begin() {
    if (!SPIFFS.begin(true)) {
        Serial.println("Error montando SPIFFS");
        return false;
    }
    
    // Bootstrap default config if not present
    if (!SPIFFS.exists(configFile)) {
        Serial.println("Archivo de configuración no encontrado. Creando configuración por defecto...");
        createDefaultConfig();
        saveConfig();
    }
    
    return loadConfig();
}

bool ConfigManager::loadConfig() {
    File file = SPIFFS.open(configFile, "r");
    if (!file) {
        Serial.println("Error abriendo archivo de configuración");
        return false;
    }
    
    StaticJsonDocument<4096> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    
    if (error) {
        Serial.print("Error parseando JSON: ");
        Serial.println(error.c_str());
        return false;
    }
    
    return parseJson(doc);
}

bool ConfigManager::saveConfig() {
    File file = SPIFFS.open(configFile, "w");
    if (!file) {
        Serial.println("Error abriendo archivo para escritura");
        return false;
    }
    
    String jsonStr = toJson();
    file.print(jsonStr);
    file.close();
    
    Serial.println("Configuración guardada correctamente");
    return true;
}

bool ConfigManager::loadFromJson(const String& jsonStr) {
    StaticJsonDocument<4096> doc;
    DeserializationError error = deserializeJson(doc, jsonStr);
    
    if (error) {
        Serial.print("Error parseando JSON: ");
        Serial.println(error.c_str());
        return false;
    }
    
    return parseJson(doc);
}

String ConfigManager::toJson() {
    StaticJsonDocument<4096> doc;
    
    // System
    JsonObject system = doc.createNestedObject("system");
    system["device_name"] = systemConfig.device_name;
    system["core0_frequency"] = systemConfig.core0_frequency;
    system["core1_frequency"] = systemConfig.core1_frequency;
    
    // WiFi
    JsonObject wifi = doc.createNestedObject("wifi");
    wifi["ssid"] = wifiConfig.ssid;
    wifi["password"] = wifiConfig.password;
    wifi["ap_mode"] = wifiConfig.ap_mode;
    wifi["ap_ssid"] = wifiConfig.ap_ssid;
    wifi["ap_password"] = wifiConfig.ap_password;
    
    // Sensors
    JsonArray sensorsArray = doc.createNestedArray("sensors");
    for (const auto& sensor : sensors) {
        JsonObject sensorObj = sensorsArray.createNestedObject();
        sensorObj["id"] = sensor.id;
        sensorObj["type"] = sensor.type;
        sensorObj["pin"] = sensor.pin;
        sensorObj["mode"] = sensor.mode;
        sensorObj["sample_rate"] = sensor.sample_rate;
        sensorObj["enabled"] = sensor.enabled;
        
        if (sensor.type == "i2c") {
            sensorObj["i2c_address"] = sensor.i2c_address;
        }
        
        if (sensor.type == "uart") {
            sensorObj["uart_rx"] = sensor.uart_rx;
            sensorObj["uart_tx"] = sensor.uart_tx;
            sensorObj["uart_baudrate"] = sensor.uart_baudrate;
        }
        
        if (!sensor.extra_params.isEmpty()) {
            sensorObj["extra_params"] = sensor.extra_params;
        }
    }
    
    // Communication
    JsonObject comm = doc.createNestedObject("communication");
    comm["protocol"] = commConfig.protocol;
    
    if (commConfig.protocol == "mqtt") {
        JsonObject mqtt = comm.createNestedObject("mqtt");
        mqtt["broker"] = commConfig.mqtt.broker;
        mqtt["port"] = commConfig.mqtt.port;
        mqtt["user"] = commConfig.mqtt.user;
        mqtt["password"] = commConfig.mqtt.password;
        mqtt["topic_prefix"] = commConfig.mqtt.topic_prefix;
        mqtt["client_id"] = commConfig.mqtt.client_id;
    } else if (commConfig.protocol == "tcp") {
        JsonObject tcp = comm.createNestedObject("tcp");
        tcp["server"] = commConfig.tcp.server;
        tcp["port"] = commConfig.tcp.port;
        tcp["use_ssl"] = commConfig.tcp.use_ssl;
    } else if (commConfig.protocol == "http") {
        JsonObject http = comm.createNestedObject("http");
        http["endpoint"] = commConfig.http.endpoint;
        http["method"] = commConfig.http.method;
        http["auth_token"] = commConfig.http.auth_token;
    }
    
    String output;
    serializeJsonPretty(doc, output);
    return output;
}

bool ConfigManager::parseJson(const JsonDocument& doc) {
    // System
    if (doc.containsKey("system")) {
        systemConfig.device_name = doc["system"]["device_name"] | "ESP32-Node-01";
        systemConfig.core0_frequency = doc["system"]["core0_frequency"] | 240;
        systemConfig.core1_frequency = doc["system"]["core1_frequency"] | 240;
    }
    
    // WiFi
    if (doc.containsKey("wifi")) {
        wifiConfig.ssid = doc["wifi"]["ssid"] | "";
        wifiConfig.password = doc["wifi"]["password"] | "";
        wifiConfig.ap_mode = doc["wifi"]["ap_mode"] | true;
        wifiConfig.ap_ssid = doc["wifi"]["ap_ssid"] | "ESP32-Config";
        wifiConfig.ap_password = doc["wifi"]["ap_password"] | "config123";
    }
    
    // Sensors
    sensors.clear();
    if (doc.containsKey("sensors")) {
        JsonArrayConst sensorsArray = doc["sensors"].as<JsonArrayConst>();
        for (JsonObjectConst sensorObj : sensorsArray) {
            SensorConfig sensor;
            sensor.id = sensorObj["id"] | "";
            sensor.type = sensorObj["type"] | "digital";
            sensor.pin = sensorObj["pin"] | 0;
            sensor.mode = sensorObj["mode"] | "input";
            sensor.sample_rate = sensorObj["sample_rate"] | 1000;
            sensor.enabled = sensorObj["enabled"] | true;
            
            if (sensor.type == "i2c") {
                sensor.i2c_address = sensorObj["i2c_address"] | 0x00;
            }
            
            if (sensor.type == "uart") {
                sensor.uart_rx = sensorObj["uart_rx"] | 16;
                sensor.uart_tx = sensorObj["uart_tx"] | 17;
                sensor.uart_baudrate = sensorObj["uart_baudrate"] | 9600;
            }
            
            if (sensorObj.containsKey("extra_params")) {
                sensor.extra_params = sensorObj["extra_params"].as<String>();
            }
            
            sensors.push_back(sensor);
        }
    }
    
    // Communication
    if (doc.containsKey("communication")) {
        commConfig.protocol = doc["communication"]["protocol"] | "mqtt";
        
        if (doc["communication"].containsKey("mqtt")) {
            commConfig.mqtt.broker = doc["communication"]["mqtt"]["broker"] | "";
            commConfig.mqtt.port = doc["communication"]["mqtt"]["port"] | 1883;
            commConfig.mqtt.user = doc["communication"]["mqtt"]["user"] | "";
            commConfig.mqtt.password = doc["communication"]["mqtt"]["password"] | "";
            commConfig.mqtt.topic_prefix = doc["communication"]["mqtt"]["topic_prefix"] | "sensors/";
            commConfig.mqtt.client_id = doc["communication"]["mqtt"]["client_id"] | systemConfig.device_name;
        }
        
        if (doc["communication"].containsKey("tcp")) {
            commConfig.tcp.server = doc["communication"]["tcp"]["server"] | "";
            commConfig.tcp.port = doc["communication"]["tcp"]["port"] | 8080;
            commConfig.tcp.use_ssl = doc["communication"]["tcp"]["use_ssl"] | false;
        }
        
        if (doc["communication"].containsKey("http")) {
            commConfig.http.endpoint = doc["communication"]["http"]["endpoint"] | "";
            commConfig.http.method = doc["communication"]["http"]["method"] | "POST";
            commConfig.http.auth_token = doc["communication"]["http"]["auth_token"] | "";
        }
    }
    
    Serial.println("Configuración cargada correctamente");
    return true;
}

void ConfigManager::createDefaultConfig() {
    // Inicializar con valores por defecto
    sensors.clear();
    
    // Sensor de prueba deshabilitado (pin 23)
    SensorConfig exampleSensor;
    exampleSensor.id = "example_sensor";
    exampleSensor.type = "digital";
    exampleSensor.pin = 23;
    exampleSensor.mode = "input_pullup";
    exampleSensor.sample_rate = 1000;
    exampleSensor.enabled = false;
    sensors.push_back(exampleSensor);
    
    commConfig.protocol = "mqtt";
    commConfig.mqtt.broker = "192.168.1.100";
    commConfig.mqtt.port = 1883;
    commConfig.mqtt.topic_prefix = "sensors/";
    commConfig.mqtt.client_id = systemConfig.device_name;
}

SensorConfig* ConfigManager::getSensorById(const String& id) {
    for (auto& sensor : sensors) {
        if (sensor.id == id) {
            return &sensor;
        }
    }
    return nullptr;
}

void ConfigManager::removeSensor(const String& id) {
    sensors.erase(
        std::remove_if(sensors.begin(), sensors.end(),
            [&id](const SensorConfig& s) { return s.id == id; }),
        sensors.end()
    );
}
