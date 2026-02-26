# API Documentation

## Web API Endpoints

### GET /

Página principal con menú de configuración.

**Response:** HTML

---

### GET /api/config

Obtiene la configuración completa en formato JSON.

**Response:**
```json
{
  "system": { ... },
  "wifi": { ... },
  "sensors": [ ... ],
  "communication": { ... }
}
```

---

### POST /api/config

Actualiza la configuración completa.

**Request Body:** JSON completo de configuración

**Response:**
```json
{
  "success": true,
  "message": "Configuración guardada"
}
```

---

### GET /sensors

Página HTML de gestión de sensores.

**Response:** HTML

---

### POST /api/sensor/add

Agrega un nuevo sensor.

**Request Body (form-data):**
- `id`: ID del sensor
- `type`: Tipo de sensor (digital, analog, i2c, uart)
- `pin`: Pin GPIO

**Response:**
```json
{
  "success": true
}
```

---

### POST /api/sensor/remove

Elimina un sensor existente.

**Request Body (form-data):**
- `id`: ID del sensor a eliminar

**Response:**
```json
{
  "success": true
}
```

---

### GET /communication

Página HTML de configuración de comunicación.

**Response:** HTML

---

### POST /api/restart

Reinicia el dispositivo.

**Response:**
```json
{
  "success": true,
  "message": "Reiniciando..."
}
```

## MQTT Topics

### Publicación

El dispositivo publica datos de sensores en:

```
{topic_prefix}{sensor_id}
```

**Payload:**
```json
{
  "value": 25.3,
  "timestamp": 1234567890
}
```

### Suscripción

El dispositivo se suscribe a comandos en:

```
{topic_prefix}commands/#
```

**Comandos soportados:**

#### Reiniciar dispositivo
```
Topic: {topic_prefix}commands/restart
Payload: {}
```

#### Actualizar configuración
```
Topic: {topic_prefix}commands/config
Payload: {
  "system": { ... },
  ...
}
```

#### Habilitar/Deshabilitar sensor
```
Topic: {topic_prefix}commands/sensor/{sensor_id}
Payload: {
  "enabled": true
}
```

## TCP Protocol

### Formato de envío

```
[sensor_id]|[json_payload]\n
```

Ejemplo:
```
temperature|{"value":23.5,"timestamp":123456}\n
```

### Formato de recepción

Los comandos recibidos deben seguir el mismo formato:

```
command|{"action":"restart"}\n
```

## HTTP/HTTPS

### Request

**Method:** POST (por defecto)

**Headers:**
- Content-Type: application/json
- Authorization: Bearer {auth_token} (si está configurado)

**Body:**
```json
{
  "topic": "sensor_id",
  "data": {
    "value": 25.3,
    "timestamp": 1234567890
  }
}
```

### Response

El servidor debe responder con código HTTP 200 o 201 para indicar éxito.

## Code API (C++)

### ConfigManager

```cpp
// Inicializar
ConfigManager config;
config.begin();

// Obtener configuración
SystemConfig sys = config.getSystemConfig();
WiFiConfig wifi = config.getWiFiConfig();
auto sensors = config.getSensors();
auto comm = config.getCommConfig();

// Modificar configuración
SensorConfig newSensor;
newSensor.id = "my_sensor";
newSensor.type = "digital";
newSensor.pin = 23;
config.addSensor(newSensor);

// Guardar
config.saveConfig();
```

### TaskManager

```cpp
// Inicializar
TaskManager taskMgr;
taskMgr.begin();

// Iniciar tareas dual-core
taskMgr.startTasks();

// Enviar mensaje entre cores
CoreMessage msg;
msg.sensorId = "temp01";
msg.value = 25.3;
taskMgr.sendToCommCore(msg);

// Recibir mensaje
CoreMessage received;
if (taskMgr.receiveFromSensorCore(received, 100)) {
    // Procesar mensaje
}
```

### Sensores

```cpp
// Sensor Digital
DigitalSensor sensor("door", 23, DigitalSensor::INPUT_PULLUP_MODE);
sensor.begin();
float value = sensor.read();

// Sensor Analógico
AnalogSensor temp("temp", 34);
temp.setAveraging(10);  // Promediar 10 muestras
temp.begin();
float voltage = temp.read();

// Sensor I2C (clase base)
class MyI2CSensor : public I2CSensor {
protected:
    float readValue() override {
        // Implementar lectura específica
        uint8_t data = readRegister(0x00);
        return (float)data;
    }
};
```

### Comunicación

```cpp
// MQTT
MQTTCredentials creds;
creds.broker = "192.168.1.100";
creds.port = 1883;
MQTTComm mqtt(creds);
mqtt.connect();
mqtt.send("sensor/temp", "{\"value\":25.3}");

// TCP
TCPCredentials tcpCreds;
tcpCreds.server = "192.168.1.50";
tcpCreds.port = 5000;
TCPComm tcp(tcpCreds);
tcp.connect();
tcp.send("temp", "{\"value\":25.3}");

// HTTP
HTTPCredentials httpCreds;
httpCreds.endpoint = "https://api.example.com/data";
httpCreds.method = "POST";
HTTPComm http(httpCreds);
http.sendJSON("{\"temp\":25.3}");
```

## Event Flow

```
1. ESP32 Boot
   ↓
2. Load Configuration (SPIFFS)
   ↓
3. Initialize WiFi (AP or Client mode)
   ↓
4. Start Web Server (Port 80)
   ↓
5. Initialize TaskManager
   ↓
6. Start Core 0 Task (Sensors)
   ↓
7. Start Core 1 Task (Communication)
   ↓
8. Main Loop (Web Server handling)
```

## Core Communication Flow

```
Core 0 (Sensors)              Core 1 (Communication)
      |                               |
   Read Sensor                        |
      |                               |
   Format Data                        |
      |                               |
   Send to Queue -----------------> Receive from Queue
                                      |
                                   Format Message
                                      |
                                   Send via Protocol
                                   (MQTT/TCP/HTTP)
```
