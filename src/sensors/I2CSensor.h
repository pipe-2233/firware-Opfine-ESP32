#ifndef I2C_SENSOR_H
#define I2C_SENSOR_H

#include "BaseSensor.h"
#include <Wire.h>

class I2CSensor : public BaseSensor {
public:
    I2CSensor(const String& id, uint8_t address, uint16_t sampleRate = 1000);
    
    bool begin() override;
    float read() override;
    String getType() override { return "i2c"; }
    
    uint8_t getAddress() const { return i2cAddress; }
    
    // I2C operations
    bool isConnected();
    uint8_t readRegister(uint8_t reg);
    void writeRegister(uint8_t reg, uint8_t value);
    bool readRegisters(uint8_t reg, uint8_t* buffer, uint8_t length);
    
protected:
    uint8_t i2cAddress;
    TwoWire* wire;
    bool deviceConnected;
    
    // Hook for device-specific read implementation
    virtual float readValue() = 0;
};

#endif
