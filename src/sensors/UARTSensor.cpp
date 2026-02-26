#include "UARTSensor.h"

UARTSensor::UARTSensor(const String& id, uint8_t rxPin, uint8_t txPin, 
                       uint32_t baudrate, uint16_t sampleRate)
    : BaseSensor(id, sampleRate), rxPin(rxPin), txPin(txPin), 
      baudrate(baudrate), serial(nullptr), serialPort(2) {
    
    // Usar Serial2 por defecto (Serial0 es USB, Serial1 puede estar en uso)
    serial = &Serial2;
}

bool UARTSensor::begin() {
    Serial.printf("Inicializando sensor UART: %s (RX:%d, TX:%d, Baud:%d)\n", 
                 sensorId.c_str(), rxPin, txPin, baudrate);
    
    serial->begin(baudrate, SERIAL_8N1, rxPin, txPin);
    
    // Esperar un momento para estabilizar
    delay(100);
    
    // Limpiar buffer
    while (serial->available()) {
        serial->read();
    }
    
    return true;
}

float UARTSensor::read() {
    if (!serial->available()) {
        return NAN;
    }
    
    String data = readLine();
    if (data.length() > 0) {
        return parseData(data);
    }
    
    return NAN;
}

bool UARTSensor::available() {
    return serial->available() > 0;
}

String UARTSensor::readLine() {
    String line = "";
    unsigned long startTime = millis();
    
    while (millis() - startTime < 1000) {  // Timeout de 1 segundo
        if (serial->available()) {
            char c = serial->read();
            if (c == '\n' || c == '\r') {
                if (line.length() > 0) {
                    return line;
                }
            } else {
                line += c;
            }
        }
    }
    
    return line;
}

void UARTSensor::writeLine(const String& data) {
    serial->println(data);
}

size_t UARTSensor::readBytes(uint8_t* buffer, size_t length) {
    return serial->readBytes(buffer, length);
}

size_t UARTSensor::writeBytes(const uint8_t* buffer, size_t length) {
    return serial->write(buffer, length);
}
