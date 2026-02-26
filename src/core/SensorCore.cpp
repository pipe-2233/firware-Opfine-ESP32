#include "SensorCore.h"
#include "TaskManager.h"

extern TaskManager taskManager;

SensorCore::SensorCore() {
    lastSampleTime = 0;
    initialized = false;
}

SensorCore::~SensorCore() {
    clearSensors();
}

bool SensorCore::begin() {
    Serial.printf("[Core %d] Inicializando SensorCore...\n", xPortGetCoreID());
    
    // Sensor instances loaded via loadSensors()
    initialized = true;
    
    Serial.printf("[Core %d] SensorCore inicializado con %d sensores\n", 
                  xPortGetCoreID(), sensors.size());
    return true;
}

void SensorCore::loop() {
    if (!initialized) return;
    
    unsigned long currentTime = millis();
    
    // Leer todos los sensores habilitados
    readSensors();
    
    // Pequeño delay para no saturar el CPU
    delay(50);
}

void SensorCore::stop() {
    clearSensors();
    initialized = false;
}

void SensorCore::loadSensors(ConfigManager& config) {
    clearSensors();
    
    Serial.println("Cargando sensores desde configuración...");
    auto sensorConfigs = config.getSensors();
    
    for (const auto& sensorConfig : sensorConfigs) {
        if (!sensorConfig.enabled) continue;
        
        Serial.printf("  - Sensor: %s (Tipo: %s, Pin: %d)\n", 
                     sensorConfig.id.c_str(), 
                     sensorConfig.type.c_str(), 
                     sensorConfig.pin);
        
        // Factory pattern: Instantiate sensor by type
        // Supported: digital, analog, i2c, uart
    }
}

void SensorCore::addSensor(BaseSensor* sensor) {
    if (sensor != nullptr) {
        sensors.push_back(sensor);
    }
}

void SensorCore::clearSensors() {
    for (auto sensor : sensors) {
        delete sensor;
    }
    sensors.clear();
}

void SensorCore::readSensors() {
    for (auto sensor : sensors) {
        if (sensor->isEnabled()) {
            float value = sensor->read();
            
            // Enviar dato al core de comunicación
            publishSensorData(sensor->getId(), value);
        }
    }
}

void SensorCore::publishSensorData(const String& sensorId, float value) {
    CoreMessage msg;
    msg.sensorId = sensorId;
    msg.type = "sensor_reading";
    msg.value = value;
    msg.timestamp = millis();
    
    taskManager.sendToCommCore(msg);
}
