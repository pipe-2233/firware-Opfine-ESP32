#ifndef UART_SENSOR_H
#define UART_SENSOR_H

#include "BaseSensor.h"
#include <HardwareSerial.h>

class UARTSensor : public BaseSensor {
public:
    UARTSensor(const String& id, uint8_t rxPin, uint8_t txPin, 
               uint32_t baudrate = 9600, uint16_t sampleRate = 1000);
    
    bool begin() override;
    float read() override;
    String getType() override { return "uart"; }
    
    // UART operations
    bool available();
    String readLine();
    void writeLine(const String& data);
    size_t readBytes(uint8_t* buffer, size_t length);
    size_t writeBytes(const uint8_t* buffer, size_t length);
    
protected:
    uint8_t rxPin;
    uint8_t txPin;
    uint32_t baudrate;
    HardwareSerial* serial;
    uint8_t serialPort;  // 0, 1, o 2
    
    // Hook for sensor-specific data parsing
    virtual float parseData(const String& data) = 0;
};

#endif
