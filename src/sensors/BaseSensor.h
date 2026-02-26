#ifndef BASE_SENSOR_H
#define BASE_SENSOR_H

#include <Arduino.h>

class BaseSensor {
public:
    BaseSensor(const String& id, uint16_t sampleRate = 1000) 
        : sensorId(id), sampleRate(sampleRate), enabled(true), lastReadTime(0) {}
    
    virtual ~BaseSensor() {}
    
    // Interface virtual para subclases
    virtual bool begin() = 0;
    virtual float read() = 0;
    virtual String getType() = 0;
    
    // API pública
    String getId() const { return sensorId; }
    bool isEnabled() const { return enabled; }
    void setEnabled(bool state) { enabled = state; }
    
    uint16_t getSampleRate() const { return sampleRate; }
    void setSampleRate(uint16_t rate) { sampleRate = rate; }
    
    bool shouldRead() {
        unsigned long currentTime = millis();
        if (currentTime - lastReadTime >= sampleRate) {
            lastReadTime = currentTime;
            return true;
        }
        return false;
    }
    
protected:
    String sensorId;
    uint16_t sampleRate;  // en milisegundos
    bool enabled;
    unsigned long lastReadTime;
};

#endif
