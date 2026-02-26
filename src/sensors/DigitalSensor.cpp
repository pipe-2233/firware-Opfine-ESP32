#include "DigitalSensor.h"

DigitalSensor::DigitalSensor(const String& id, uint8_t pin, PinMode mode, uint16_t sampleRate)
    : BaseSensor(id, sampleRate), pin(pin), pinMode(mode), currentState(false) {
}

bool DigitalSensor::begin() {
    Serial.printf("Inicializando sensor digital: %s en pin %d\n", sensorId.c_str(), pin);
    
    switch (pinMode) {
        case INPUT_MODE:
            ::pinMode(pin, INPUT);
            break;
        case INPUT_PULLUP_MODE:
            ::pinMode(pin, INPUT_PULLUP);
            break;
        case INPUT_PULLDOWN_MODE:
            ::pinMode(pin, INPUT_PULLDOWN);
            break;
    }
    
    return true;
}

float DigitalSensor::read() {
    currentState = digitalRead(pin);
    return currentState ? 1.0 : 0.0;
}
