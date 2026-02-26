#ifndef DIGITAL_SENSOR_H
#define DIGITAL_SENSOR_H

#include "BaseSensor.h"

class DigitalSensor : public BaseSensor {
public:
    enum PinMode {
        INPUT_MODE,
        INPUT_PULLUP_MODE,
        INPUT_PULLDOWN_MODE
    };
    
    DigitalSensor(const String& id, uint8_t pin, PinMode mode = INPUT_MODE, uint16_t sampleRate = 100);
    
    bool begin() override;
    float read() override;
    String getType() override { return "digital"; }
    
    uint8_t getPin() const { return pin; }
    bool getState() const { return currentState; }
    
private:
    uint8_t pin;
    PinMode pinMode;
    bool currentState;
};

#endif
