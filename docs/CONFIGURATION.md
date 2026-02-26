# Guía de Configuración - ESP32 Modular Firmware

## Tabla de Contenidos
- [Introducción](#introducción)
- [Configuración del Sistema](#configuración-del-sistema)
- [Configuración WiFi](#configuración-wifi)
- [Configuración de Sensores](#configuración-de-sensores)
- [Configuración de Comunicación](#configuración-de-comunicación)
- [Ejemplos](#ejemplos)

## Introducción

Este firmware soporta dos modos de configuración:

1. **Modo Fácil**: Interfaz web accesible en `http://192.168.4.1` (en modo AP)
2. **Modo Avanzado**: Edición directa del archivo `data/config.json`

## Configuración del Sistema

```json
{
  "system": {
    "device_name": "ESP32-Node-01",
    "core0_frequency": 240,
    "core1_frequency": 240
  }
}
```

### Parámetros:
- **device_name**: Nombre único del dispositivo (usado en MQTT y logs)
- **core0_frequency**: Frecuencia del Core 0 en MHz (80, 160, 240)
- **core1_frequency**: Frecuencia del Core 1 en MHz (80, 160, 240)

## Configuración WiFi

```json
{
  "wifi": {
    "ssid": "MiRedWiFi",
    "password": "miPassword",
    "ap_mode": false,
    "ap_ssid": "ESP32-Config",
    "ap_password": "config123"
  }
}
```

### Parámetros:
- **ssid**: Nombre de la red WiFi a la que conectarse
- **password**: Contraseña de la red WiFi
- **ap_mode**: `true` para modo Access Point, `false` para modo cliente
- **ap_ssid**: SSID del Access Point (cuando ap_mode = true)
- **ap_password**: Contraseña del Access Point

### Comportamiento:
- Si `ap_mode = true`, la ESP32 crea su propia red WiFi
- Si `ap_mode = false` pero no puede conectarse, cambia automáticamente a modo AP
- En modo AP, puedes acceder a `http://192.168.4.1` para configurar

## Configuración de Sensores

Los sensores se configuran en el array `sensors`:

### Sensor Digital

```json
{
  "id": "door_sensor",
  "type": "digital",
  "pin": 23,
  "mode": "input_pullup",
  "sample_rate": 100,
  "enabled": true
}
```

**Parámetros:**
- **id**: Identificador único del sensor
- **type**: `"digital"`
- **pin**: Pin GPIO a usar
- **mode**: `"input"`, `"input_pullup"`, o `"input_pulldown"`
- **sample_rate**: Intervalo de lectura en milisegundos
- **enabled**: `true` para habilitar, `false` para deshabilitar

### Sensor Analógico

```json
{
  "id": "temperature",
  "type": "analog",
  "pin": 34,
  "sample_rate": 1000,
  "enabled": true,
  "extra_params": "{\"calibration\":{\"min\":0,\"max\":3.3}}"
}
```

**Parámetros:**
- **pin**: Pin ADC (34, 35, 32, 33, 25, 26, 27, 14, 12, 13)
- **extra_params**: JSON string con parámetros adicionales de calibración

**Pines ADC recomendados:**
- ADC1: 32, 33, 34, 35, 36, 39 (mejor opción, no conflicto con WiFi)
- ADC2: 0, 2, 4, 12-15, 25-27 (no usar con WiFi activo)

### Sensor I2C

```json
{
  "id": "bme280_sensor",
  "type": "i2c",
  "pin": 0,
  "i2c_address": 118,
  "sample_rate": 5000,
  "enabled": true,
  "extra_params": "{\"sensor_model\":\"BME280\"}"
}
```

**Parámetros:**
- **i2c_address**: Dirección I2C del dispositivo (decimal)
- **extra_params**: Parámetros específicos del sensor

**Pines I2C por defecto:**
- SDA: GPIO 21
- SCL: GPIO 22

### Sensor UART

```json
{
  "id": "gps_module",
  "type": "uart",
  "pin": 0,
  "uart_rx": 16,
  "uart_tx": 17,
  "uart_baudrate": 9600,
  "sample_rate": 1000,
  "enabled": true,
  "extra_params": "{\"protocol\":\"NMEA\"}"
}
```

**Parámetros:**
- **uart_rx**: Pin RX (recepción)
- **uart_tx**: Pin TX (transmisión)
- **uart_baudrate**: Velocidad de comunicación (9600, 115200, etc.)

## Configuración de Comunicación

### MQTT

```json
{
  "communication": {
    "protocol": "mqtt",
    "mqtt": {
      "broker": "192.168.1.100",
      "port": 1883,
      "user": "mqtt_user",
      "password": "SecureMqtt2024",
      "topic_prefix": "sensors/",
      "client_id": "ESP32-Node-01"
    }
  }
}
```

**Parámetros:**
- **broker**: Dirección IP o hostname del broker MQTT
- **port**: Puerto (1883 por defecto, 8883 para TLS)
- **user**: Usuario (dejar vacío si no requiere autenticación)
- **password**: Contraseña
- **topic_prefix**: Prefijo para todos los topics publicados
- **client_id**: ID único del cliente MQTT

**Topics publicados:**
- `{topic_prefix}{sensor_id}`: Datos del sensor

**Topics suscritos:**
- `{topic_prefix}commands/#`: Comandos para el dispositivo

### TCP/IP

```json
{
  "communication": {
    "protocol": "tcp",
    "tcp": {
      "server": "192.168.1.50",
      "port": 5000,
      "use_ssl": false
    }
  }
}
```

**Parámetros:**
- **server**: Dirección IP o hostname del servidor
- **port**: Puerto TCP
- **use_ssl**: `true` para usar SSL/TLS, `false` para TCP plano

**Formato de datos enviados:**
```
[sensor_id]|{"value":25.3,"timestamp":12345}\n
```

### HTTP/HTTPS

```json
{
  "communication": {
    "protocol": "http",
    "http": {
      "endpoint": "https://api.example.com/sensors",
      "method": "POST",
      "auth_token": "Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJlc3AzMi1kZXZpY2UifQ.SflKxwRJ"
    }
  }
}
```

**Parámetros:**
- **endpoint**: URL completa del endpoint
- **method**: `"GET"`, `"POST"`, o `"PUT"`
- **auth_token**: Token de autenticación (opcional)

**Formato de datos enviados:**
```json
{
  "topic": "sensor_id",
  "data": {
    "value": 25.3,
    "timestamp": 12345
  }
}
```

## Ejemplos

Ver la carpeta `examples/` para configuraciones completas:
- `basic_config.json`: Configuración básica con 2 sensores y MQTT
- `advanced_config.json`: Configuración avanzada con múltiples sensores y tipos
- `http_config.json`: Ejemplo usando HTTP para enviar datos
- `tcp_config.json`: Ejemplo usando TCP/IP

## Validación de Configuración

El firmware valida automáticamente la configuración al inicio. Si hay errores:
1. Revisa el monitor serial para ver los mensajes de error
2. Verifica que el JSON sea válido (usa un validador online)
3. Confirma que los pines no estén en conflicto
4. Asegúrate de que los valores estén en rangos válidos

## Respaldo de Configuración

Antes de hacer cambios importantes, haz un respaldo:

```bash
# Descargar configuración actual
curl http://192.168.4.1/api/config > backup_config.json

# Restaurar configuración
curl -X POST -H "Content-Type: application/json" \
  -d @backup_config.json \
  http://192.168.4.1/api/config
```
