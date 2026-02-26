#include "I2CSensor.h"

I2CSensor::I2CSensor(const String& id, uint8_t address, uint16_t sampleRate)
    : BaseSensor(id, sampleRate), i2cAddress(address), wire(&Wire), deviceConnected(false) {
}

bool I2CSensor::begin() {
    Serial.printf("Inicializando sensor I2C: %s en dirección 0x%02X\n", sensorId.c_str(), i2cAddress);
    
    // Verificar si el dispositivo está conectado
    deviceConnected = isConnected();
    
    if (!deviceConnected) {
        Serial.printf("Advertencia: Sensor I2C %s no encontrado en 0x%02X\n", 
                     sensorId.c_str(), i2cAddress);
        return false;
    }
    
    return true;
}

float I2CSensor::read() {
    if (!deviceConnected) {
        // Intentar reconectar
        deviceConnected = isConnected();
        if (!deviceConnected) {
            return NAN;
        }
    }
    
    return readValue();
}

bool I2CSensor::isConnected() {
    wire->beginTransmission(i2cAddress);
    return (wire->endTransmission() == 0);
}

uint8_t I2CSensor::readRegister(uint8_t reg) {
    wire->beginTransmission(i2cAddress);
    wire->write(reg);
    wire->endTransmission();
    
    wire->requestFrom(i2cAddress, (uint8_t)1);
    if (wire->available()) {
        return wire->read();
    }
    return 0;
}

void I2CSensor::writeRegister(uint8_t reg, uint8_t value) {
    wire->beginTransmission(i2cAddress);
    wire->write(reg);
    wire->write(value);
    wire->endTransmission();
}

bool I2CSensor::readRegisters(uint8_t reg, uint8_t* buffer, uint8_t length) {
    wire->beginTransmission(i2cAddress);
    wire->write(reg);
    wire->endTransmission();
    
    wire->requestFrom(i2cAddress, length);
    
    uint8_t count = 0;
    while (wire->available() && count < length) {
        buffer[count++] = wire->read();
    }
    
    return count == length;
}
