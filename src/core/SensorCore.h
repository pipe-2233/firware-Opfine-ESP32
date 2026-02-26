#ifndef SENSOR_CORE_H
#define SENSOR_CORE_H

#include <Arduino.h>
#include <vector>
#include "../config/ConfigManager.h"
#include "../sensors/BaseSensor.h"

class SensorCore {
public:
    SensorCore();
    ~SensorCore();
    
    bool begin();
    void loop();
    void stop();
    
    void loadSensors(ConfigManager& config);
    void addSensor(BaseSensor* sensor);
    void clearSensors();
    
private:
    std::vector<BaseSensor*> sensors;
    unsigned long lastSampleTime;
    bool initialized;
    
    void readSensors();
    void publishSensorData(const String& sensorId, float value);
};

#endif
