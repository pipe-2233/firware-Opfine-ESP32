# Agregar Nuevos Sensores

Esta guía explica cómo agregar soporte para nuevos tipos de sensores al firmware.

## Estructura de un Sensor

Todos los sensores deben heredar de la clase base `BaseSensor`:

```cpp
class BaseSensor {
public:
    virtual bool begin() = 0;      // Inicialización
    virtual float read() = 0;       // Lectura de datos
    virtual String getType() = 0;   // Tipo de sensor
    
    // Métodos comunes ya implementados
    String getId();
    bool isEnabled();
    void setEnabled(bool state);
    uint16_t getSampleRate();
    void setSampleRate(uint16_t rate);
    bool shouldRead();
};
```

## Paso 1: Crear la Clase del Sensor

Crea dos archivos en `src/sensors/`:
- `MySensor.h` (header)
- `MySensor.cpp` (implementación)

### Ejemplo: Sensor de Distancia Ultrasónico (HC-SR04)

**src/sensors/UltrasonicSensor.h**
```cpp
#ifndef ULTRASONIC_SENSOR_H
#define ULTRASONIC_SENSOR_H

#include "BaseSensor.h"

class UltrasonicSensor : public BaseSensor {
public:
    UltrasonicSensor(const String& id, uint8_t trigPin, uint8_t echoPin, 
                     uint16_t sampleRate = 200);
    
    bool begin() override;
    float read() override;
    String getType() override { return "ultrasonic"; }
    
    float getDistance() const { return distance; }
    void setMaxDistance(float max) { maxDistance = max; }
    
private:
    uint8_t trigPin;
    uint8_t echoPin;
    float distance;
    float maxDistance;
    
    long measurePulse();
};

#endif
```

**src/sensors/UltrasonicSensor.cpp**
```cpp
#include "UltrasonicSensor.h"

UltrasonicSensor::UltrasonicSensor(const String& id, uint8_t trig, uint8_t echo, 
                                   uint16_t sampleRate)
    : BaseSensor(id, sampleRate), trigPin(trig), echoPin(echo), 
      distance(0), maxDistance(400.0) {
}

bool UltrasonicSensor::begin() {
    Serial.printf("Inicializando sensor ultrasónico: %s (TRIG:%d, ECHO:%d)\n", 
                 sensorId.c_str(), trigPin, echoPin);
    
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
    
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    
    return true;
}

float UltrasonicSensor::read() {
    long duration = measurePulse();
    
    // Calcular distancia en centímetros
    // Velocidad del sonido: 343 m/s = 0.0343 cm/µs
    // Distancia = (tiempo * velocidad) / 2 (ida y vuelta)
    distance = (duration * 0.0343) / 2.0;
    
    // Limitar al rango máximo
    if (distance > maxDistance) {
        distance = maxDistance;
    }
    
    return distance;
}

long UltrasonicSensor::measurePulse() {
    // Enviar pulso
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    
    // Medir tiempo del eco
    long duration = pulseIn(echoPin, HIGH, 30000); // 30ms timeout
    
    return duration;
}
```

## Paso 2: Integrar con ConfigManager

Modifica `src/core/SensorCore.cpp` para agregar soporte al nuevo sensor:

```cpp
#include "UltrasonicSensor.h"

void SensorCore::loadSensors(ConfigManager& config) {
    clearSensors();
    
    auto sensorConfigs = config.getSensors();
    
    for (const auto& sensorConfig : sensorConfigs) {
        if (!sensorConfig.enabled) continue;
        
        BaseSensor* sensor = nullptr;
        
        if (sensorConfig.type == "digital") {
            // ... código existente ...
        } 
        else if (sensorConfig.type == "analog") {
            // ... código existente ...
        }
        else if (sensorConfig.type == "ultrasonic") {
            // Nuevo sensor ultrasónico
            // Esperamos que extra_params contenga el pin ECHO
            StaticJsonDocument<128> doc;
            deserializeJson(doc, sensorConfig.extra_params);
            uint8_t echoPin = doc["echo_pin"] | 0;
            
            sensor = new UltrasonicSensor(
                sensorConfig.id,
                sensorConfig.pin,  // TRIG pin
                echoPin,
                sensorConfig.sample_rate
            );
        }
        
        if (sensor != nullptr) {
            if (sensor->begin()) {
                addSensor(sensor);
                Serial.printf("✓ Sensor %s cargado\n", sensorConfig.id.c_str());
            } else {
                Serial.printf("✗ Error cargando sensor %s\n", sensorConfig.id.c_str());
                delete sensor;
            }
        }
    }
}
```

## Paso 3: Documentar la Configuración

Agrega un ejemplo de configuración JSON:

```json
{
  "id": "distance_sensor",
  "type": "ultrasonic",
  "pin": 5,
  "sample_rate": 200,
  "enabled": true,
  "extra_params": "{\"echo_pin\":18,\"max_distance\":400}"
}
```

## Ejemplo Avanzado: Sensor I2C (BME280)

Para sensores I2C más complejos:

**src/sensors/BME280Sensor.h**
```cpp
#ifndef BME280_SENSOR_H
#define BME280_SENSOR_H

#include "I2CSensor.h"

class BME280Sensor : public I2CSensor {
public:
    BME280Sensor(const String& id, uint8_t address = 0x76, 
                 uint16_t sampleRate = 5000);
    
    bool begin() override;
    
    float getTemperature() const { return temperature; }
    float getHumidity() const { return humidity; }
    float getPressure() const { return pressure; }
    
protected:
    float readValue() override;
    
private:
    float temperature;
    float humidity;
    float pressure;
    
    // Registros del BME280
    static const uint8_t REG_TEMP_MSB = 0xFA;
    static const uint8_t REG_HUM_MSB = 0xFD;
    static const uint8_t REG_PRESS_MSB = 0xF7;
    
    void readAllData();
};

#endif
```

**src/sensors/BME280Sensor.cpp**
```cpp
#include "BME280Sensor.h"

BME280Sensor::BME280Sensor(const String& id, uint8_t address, uint16_t sampleRate)
    : I2CSensor(id, address, sampleRate), 
      temperature(0), humidity(0), pressure(0) {
}

bool BME280Sensor::begin() {
    if (!I2CSensor::begin()) {
        return false;
    }
    
    // Configurar el BME280
    writeRegister(0xF2, 0x01);  // Humidity oversampling x1
    writeRegister(0xF4, 0x27);  // Temp/Press oversampling, normal mode
    writeRegister(0xF5, 0xA0);  // Config register
    
    delay(100);
    
    Serial.println("BME280 inicializado correctamente");
    return true;
}

float BME280Sensor::readValue() {
    readAllData();
    return temperature;  // Por defecto retornamos temperatura
}

void BME280Sensor::readAllData() {
    uint8_t buffer[8];
    
    // Leer todos los datos de una vez
    if (readRegisters(REG_PRESS_MSB, buffer, 8)) {
        // Parsear datos (simplificado, en producción usar las fórmulas de compensación)
        int32_t adc_P = ((uint32_t)buffer[0] << 12) | 
                       ((uint32_t)buffer[1] << 4) | 
                       ((buffer[2] >> 4) & 0x0F);
        
        int32_t adc_T = ((uint32_t)buffer[3] << 12) | 
                       ((uint32_t)buffer[4] << 4) | 
                       ((buffer[5] >> 4) & 0x0F);
        
        int32_t adc_H = ((uint32_t)buffer[6] << 8) | buffer[7];
        
        // Aplicar compensación (aquí versión simplificada)
        temperature = adc_T / 100.0;
        humidity = adc_H / 100.0;
        pressure = adc_P / 100.0;
    }
}
```

## Checklist de Integración

- [ ] Crear clase heredando de `BaseSensor` (o subclases como `I2CSensor`)
- [ ] Implementar métodos: `begin()`, `read()`, `getType()`
- [ ] Agregar lógica de carga en `SensorCore::loadSensors()`
- [ ] Documentar configuración JSON
- [ ] Crear ejemplo de configuración en `examples/`
- [ ] Probar con datos reales
- [ ] Actualizar README con el nuevo sensor

## Tips

1. **Manejo de errores**: Usa `return NAN;` si falla la lectura
2. **Timeouts**: Implementa timeouts para evitar bloqueos
3. **Calibración**: Permite parámetros de calibración en `extra_params`
4. **Logging**: Usa `Serial.printf()` para debug
5. **Recursos**: Libera recursos en el destructor
6. **Thread-safe**: Considera usar mutex si accedes a recursos compartidos

## Ejemplos de Sensores Populares

### DHT22 (Temperatura/Humedad)
```cpp
class DHT22Sensor : public BaseSensor {
    // Usar librería DHT-sensor-library
};
```

### DS18B20 (OneWire)
```cpp
class DS18B20Sensor : public BaseSensor {
    // Usar librería OneWire y DallasTemperature
};
```

### MPU6050 (Acelerómetro/Giroscopio)
```cpp
class MPU6050Sensor : public I2CSensor {
    // Leer registros de aceleración y giroscopio
};
```

¿Necesitas ayuda con un sensor específico? ¡Crea un issue en GitHub!
