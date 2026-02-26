#ifndef ANALOG_SENSOR_H
#define ANALOG_SENSOR_H

#include "BaseSensor.h"

class AnalogSensor : public BaseSensor {
public:
    AnalogSensor(const String& id, uint8_t pin, uint16_t sampleRate = 1000);
    
    bool begin() override;
    float read() override;
    String getType() override { return "analog"; }
    
    uint8_t getPin() const { return pin; }
    uint16_t getRawValue() const { return rawValue; }
    float getVoltage() const { return voltage; }
    
    // Configuración de calibración
    void setCalibration(float minVoltage, float maxVoltage, float minValue, float maxValue);
    void setAveraging(uint8_t samples);
    
private:
    uint8_t pin;
    uint16_t rawValue;
    float voltage;
    
    // Calibración
    bool useCalibration;
    float minVoltage, maxVoltage;
    float minValue, maxValue;
    
    // Promediado
    uint8_t averageSamples;
    
    float mapValue(float value);
};

#endif
