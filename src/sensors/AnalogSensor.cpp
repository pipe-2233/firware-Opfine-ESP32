#include "AnalogSensor.h"

AnalogSensor::AnalogSensor(const String& id, uint8_t pin, uint16_t sampleRate)
    : BaseSensor(id, sampleRate), pin(pin), rawValue(0), voltage(0.0),
      useCalibration(false), averageSamples(1) {
}

bool AnalogSensor::begin() {
    Serial.printf("Inicializando sensor analógico: %s en pin %d\n", sensorId.c_str(), pin);
    
    // Configurar resolución ADC (ESP32 por defecto es 12 bits)
    analogReadResolution(12);
    
    // Configurar atenuación (por defecto 11dB para rango completo 0-3.3V)
    analogSetAttenuation(ADC_11db);
    
    return true;
}

float AnalogSensor::read() {
    uint32_t sum = 0;
    
    // Leer múltiples muestras para promediado
    for (uint8_t i = 0; i < averageSamples; i++) {
        sum += analogRead(pin);
        if (averageSamples > 1) {
            delayMicroseconds(100);
        }
    }
    
    rawValue = sum / averageSamples;
    
    // Convertir a voltaje (ESP32: 12 bits, 0-3.3V)
    voltage = (rawValue / 4095.0) * 3.3;
    
    // Aplicar calibración si está configurada
    if (useCalibration) {
        return mapValue(voltage);
    }
    
    return voltage;
}

void AnalogSensor::setCalibration(float minV, float maxV, float minVal, float maxVal) {
    minVoltage = minV;
    maxVoltage = maxV;
    minValue = minVal;
    maxValue = maxVal;
    useCalibration = true;
}

void AnalogSensor::setAveraging(uint8_t samples) {
    averageSamples = constrain(samples, 1, 50);
}

float AnalogSensor::mapValue(float value) {
    // Mapear el voltaje al rango calibrado
    if (value <= minVoltage) return minValue;
    if (value >= maxVoltage) return maxValue;
    
    return minValue + (value - minVoltage) * (maxValue - minValue) / (maxVoltage - minVoltage);
}
