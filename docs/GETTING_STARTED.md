# Guía de Inicio Rápido

## 🚀 Primeros Pasos

### 1. Requisitos Previos

**Hardware:**
- ESP32 DevKit o compatible
- Cable USB
- Sensores (opcional para empezar)

**Software:**
- [Visual Studio Code](https://code.visualstudio.com/)
- [PlatformIO Extension](https://platformio.org/install/ide?install=vscode)
- Git

### 2. Clonar el Repositorio

```bash
git clone https://github.com/andresfirmware/esp32-modular-firmware.git
cd esp32-modular-firmware
```

### 3. Abrir en VS Code

```bash
code .
```

### 4. Compilar el Proyecto

1. Espera a que PlatformIO se inicialice
2. Click en el ícono de PlatformIO en la barra lateral
3. Click en "Build" bajo "esp32dev"

O desde la terminal:

```bash
pio run
```

### 5. Subir a la ESP32

1. Conecta tu ESP32 al puerto USB
2. En PlatformIO, click en "Upload"

O desde terminal:

```bash
pio run --target upload
```

### 6. Monitorear el Serial

```bash
pio device monitor -b 115200
```

Deberías ver algo como:

```
=================================
ESP32 Modular Firmware v1.0
=================================

1. Cargando configuración...
2. Configurando WiFi...
   Iniciando modo Access Point...
   SSID: ESP32-Config
   IP del AP: 192.168.4.1
3. Iniciando servidor web...
4. Inicializando gestor de tareas...
5. Iniciando tareas dual-core...

=================================
Sistema Iniciado Exitosamente
=================================
```

### 7. Acceder a la Interfaz Web

1. Conéctate a la red WiFi: `ESP32-Config`
2. Contraseña: `config123`
3. Abre tu navegador en: `http://192.168.4.1`

## 📱 Primer Proyecto: Sensor de Temperatura

### Hardware Necesario
- ESP32
- Sensor de temperatura analógico (ej: LM35)
- Cables

### Conexiones
```
LM35 VCC  → ESP32 3.3V
LM35 OUT  → ESP32 GPIO 34
LM35 GND  → ESP32 GND
```

### Configuración

1. Copia este JSON en `data/config.json`:

```json
{
  "system": {
    "device_name": "ESP32-Temperature-Monitor",
    "core0_frequency": 240,
    "core1_frequency": 240
  },
  "wifi": {
    "ssid": "TuRedWiFi",
    "password": "TuPassword",
    "ap_mode": false,
    "ap_ssid": "ESP32-Config",
    "ap_password": "config123"
  },
  "sensors": [
    {
      "id": "temperature",
      "type": "analog",
      "pin": 34,
      "sample_rate": 2000,
      "enabled": true
    }
  ],
  "communication": {
    "protocol": "mqtt",
    "mqtt": {
      "broker": "test.mosquitto.org",
      "port": 1883,
      "user": "",
      "password": "",
      "topic_prefix": "esp32/temp/",
      "client_id": "ESP32-Temperature-Monitor"
    }
  }
}
```

2. Sube el firmware y el filesystem:

```bash
pio run --target upload
pio run --target uploadfs
```

3. Monitorea los datos en MQTT:

```bash
# Instala mosquitto-clients
sudo apt install mosquitto-clients

# Suscríbete al topic
mosquitto_sub -h test.mosquitto.org -t "esp32/temp/#"
```

## 🔧 Configuración Avanzada

### Agregar Múltiples Sensores

```json
{
  "sensors": [
    {
      "id": "temperature",
      "type": "analog",
      "pin": 34,
      "sample_rate": 2000,
      "enabled": true
    },
    {
      "id": "motion",
      "type": "digital",
      "pin": 23,
      "mode": "input_pullup",
      "sample_rate": 100,
      "enabled": true
    },
    {
      "id": "light",
      "type": "analog",
      "pin": 35,
      "sample_rate": 5000,
      "enabled": true
    }
  ]
}
```

### Usar HTTP en lugar de MQTT

```json
{
  "communication": {
    "protocol": "http",
    "http": {
      "endpoint": "https://tu-servidor.com/api/sensors",
      "method": "POST",
      "auth_token": "Bearer TU_TOKEN_AQUI"
    }
  }
}
```

### Modo Debug

Para ver más información de debug, modifica `platformio.ini`:

```ini
build_flags = 
    -DCORE_DEBUG_LEVEL=5
```

Niveles de debug:
- 0 = None
- 1 = Error
- 2 = Warning
- 3 = Info
- 4 = Debug
- 5 = Verbose

## 🐛 Solución de Problemas

### Error: No se encuentra el puerto

```bash
# Linux: Agregar permisos
sudo usermod -a -G dialout $USER
# Reinicia sesión

# Ver puertos disponibles
pio device list
```

### Error: SPIFFS mount failed

El sistema creará automáticamente una configuración por defecto.

### WiFi no conecta

1. Verifica SSID y password
2. Comprueba que la red sea 2.4GHz (ESP32 no soporta 5GHz)
3. Revisa el monitor serial para ver errores

### No puedo acceder a la interfaz web

1. Verifica que estés conectado al AP de la ESP32
2. Intenta `192.168.4.1` en el navegador
3. Desactiva datos móviles en el dispositivo

## 📚 Siguientes Pasos

1. Lee la [Guía de Configuración Completa](docs/CONFIGURATION.md)
2. Aprende a [Agregar Nuevos Sensores](docs/ADDING_SENSORS.md)
3. Explora [Ejemplos Avanzados](examples/)
4. Revisa la [Documentación de API](docs/API.md)

## 💡 Ejemplos Rápidos

### Leer un sensor digital

```cpp
DigitalSensor door("door", 23, DigitalSensor::INPUT_PULLUP_MODE);
door.begin();
float value = door.read();  // 1.0 = HIGH, 0.0 = LOW
```

### Leer un sensor analógico

```cpp
AnalogSensor temp("temp", 34);
temp.setAveraging(10);  // Promediar 10 lecturas
temp.begin();
float voltage = temp.read();
```

### Enviar datos por MQTT

```cpp
MQTTCredentials creds;
creds.broker = "test.mosquitto.org";
creds.port = 1883;

MQTTComm mqtt(creds);
mqtt.connect();
mqtt.send("temperature", "{\"value\":25.3}");
```

## 🎯 Proyectos de Ejemplo

### Estación Meteorológica
- Temperatura (LM35)
- Humedad (DHT22)
- Presión (BMP280)
- Envío por MQTT

### Sistema de Seguridad
- PIR (Movimiento)
- Contacto magnético (Puertas)
- Buzzer (Alarma)
- Notificación por HTTP

### Monitor de Plantas
- Humedad de suelo (Analógico)
- Luz (LDR)
- Temperatura ambiente
- Dashboard web

## 🤝 Contribuir

¿Mejoraste algo? ¡Compártelo!

1. Fork el proyecto
2. Crea una rama: `git checkout -b feature/MiMejora`
3. Commit: `git commit -m 'Agregar MiMejora'`
4. Push: `git push origin feature/MiMejora`
5. Pull Request

## 🆘 Obtener Ayuda

- [Issues en GitHub](https://github.com/andresfirmware/esp32-modular-firmware/issues)
- [Discussions](https://github.com/andresfirmware/esp32-modular-firmware/discussions)
- [Wiki del Proyecto](https://github.com/andresfirmware/esp32-modular-firmware/wiki)
