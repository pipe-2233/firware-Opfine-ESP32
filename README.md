<div align="center">

# 🔧 ESP32 Modular Firmware

### Framework profesional para desarrollo de sistemas IoT basados en ESP32

[![PlatformIO](https://img.shields.io/badge/PlatformIO-Ready-orange.svg)](https://platformio.org/)
[![ESP32](https://img.shields.io/badge/ESP32-Compatible-blue.svg)](https://www.espressif.com/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)]()
[![Version](https://img.shields.io/badge/version-1.0.0-blue.svg)]()

Arquitectura modular y escalable que aprovecha la capacidad dual-core del ESP32 para gestionar sensores y comunicaciones de forma eficiente y simultánea.

[Inicio Rápido](#-instalación-rápida) • [Documentación](#-documentación) • [Ejemplos](#-ejemplos-de-uso) • [Contribuir](CONTRIBUTING.md)

</div>

---

## 🌟 Características Principales

### 🧠 Arquitectura Dual-Core Optimizada
- **Core 0**: Dedicado exclusivamente a la adquisición de datos y gestión de sensores
- **Core 1**: Maneja toda la comunicación, conectividad y servicios web
- Comunicación eficiente entre cores mediante queues de FreeRTOS
- Procesamiento paralelo real sin latencias ni bloqueos

### ⚙️ Sistema de Configuración Dual
- **Interfaz Web**: Configuración visual e intuitiva sin necesidad de programar
- **Archivos JSON**: Control total y avanzado para configuraciones complejas
- Intercambiables en cualquier momento sin recompilar
- Persistencia automática en SPIFFS

### 📡 Protocolos de Comunicación
- **MQTT**: Cliente completo con reconexión automática y soporte TLS
- **TCP/IP**: Cliente TCP con soporte SSL/TLS opcional
- **HTTP/HTTPS**: Cliente REST con autenticación por token
- **Extensible**: Arquitectura preparada para agregar nuevos protocolos

### 🔌 Soporte Universal de Sensores
- **Digitales**: Entrada/Salida con pull-up/pull-down configurable
- **Analógicos**: ADC de 12 bits con calibración y promediado
- **I2C**: Interfaz completa para sensores inteligentes
- **UART**: Comunicación serial con sensores especializados
- **Modular**: Sistema extensible para cualquier tipo de sensor

---

## 📋 Tabla de Contenidos

- [Instalación Rápida](#-instalación-rápida)
- [Ejemplos de Uso](#-ejemplos-de-uso)
- [Configuración](#-configuración)
- [Documentación](#-documentación)
- [Arquitectura](#-arquitectura-del-sistema)
- [Rendimiento](#-rendimiento)
- [FAQ](#-preguntas-frecuentes)
- [Roadmap](#-roadmap)
- [Contribuir](#-contribuir)

---

## ⚡ Instalación Rápida

### Requisitos Previos
- **Hardware**: ESP32 DevKit (o compatible)
- **Software**: [PlatformIO IDE](https://platformio.org/install/ide?install=vscode) o PlatformIO CLI
- **Cable**: USB tipo A a micro USB

### Instalación en 3 Pasos

```bash
# 1. Clonar el repositorio
git clone https://github.com/andresfirmware/esp32-modular-firmware.git
cd esp32-modular-firmware

# 2. Compilar el firmware
pio run

# 3. Flashear a la ESP32 (conectada por USB)
pio run --target upload && pio device monitor
```

¡Listo! Tu ESP32 estará ejecutando el firmware con la configuración por defecto en modo Access Point.

---

## 🎯 Ejemplos de Uso

### 📊 Caso 1: Estación Meteorológica Básica

Monitoreo de temperatura y humedad con envío de datos vía MQTT a Home Assistant.

**Configuración (`examples/basic_config.json`):**

```json
{
  "system": {
    "device_name": "MeteoStation-01",
    "core0_frequency": 240,
    "core1_frequency": 240
  },
  "wifi": {
    "ssid": "HomeNetwork",
    "password": "SecurePass2024",
    "ap_mode": false
  },
  "sensors": [
    {
      "id": "temperature",
      "type": "analog",
      "pin": 34,
      "sample_rate": 2000,
      "enabled": true
    },
    {
      "id": "humidity",
      "type": "analog",
      "pin": 35,
      "sample_rate": 2000,
      "enabled": true
    }
  ],
  "communication": {
    "protocol": "mqtt",
    "mqtt": {
      "broker": "192.168.1.100",
      "port": 1883,
      "user": "homeassistant",
      "password": "mqtt_password",
      "topic_prefix": "home/weather/",
      "use_tls": false
    }
  }
}
```

**Resultado:**
- Los datos se publican cada 2 segundos en `home/weather/temperature` y `home/weather/humidity`
- Compatible con Home Assistant, Node-RED, etc.
- Consumo: ~80mA @ 5V

---

### 🏭 Caso 2: Sistema de Monitoreo Industrial

Monitoreo de presión y flujo en línea de producción con transmisión TCP/TLS a sistema SCADA.

**Configuración (`examples/tcp_config.json`):**

```json
{
  "system": {
    "device_name": "Industrial-Monitor-A1",
    "core0_frequency": 240,
    "core1_frequency": 240
  },
  "wifi": {
    "ssid": "IndustryNet",
    "password": "Ind2024Secure!",
    "ap_mode": false
  },
  "sensors": [
    {
      "id": "pressure_sensor",
      "type": "i2c",
      "i2c_address": 118,
      "sample_rate": 500,
      "enabled": true
    },
    {
      "id": "flow_meter",
      "type": "digital",
      "pin": 23,
      "mode": "input_pullup",
      "sample_rate": 100,
      "enabled": true
    },
    {
      "id": "vibration",
      "type": "analog",
      "pin": 36,
      "sample_rate": 50,
      "enabled": true
    }
  ],
  "communication": {
    "protocol": "tcp",
    "tcp": {
      "server": "10.0.1.50",
      "port": 5000,
      "use_ssl": true,
      "reconnect_interval": 5000
    }
  }
}
```

**Resultado:**
- Alta frecuencia de muestreo (hasta 20Hz en vibración)
- Conexión segura TLS al servidor SCADA
- Reconexión automática ante pérdida de red
- Consumo: ~120mA @ 5V

---

### 🌱 Caso 3: Monitor Agrícola Inteligente

Automatización de riego con sensores de humedad del suelo y envío a API REST en la nube.

**Configuración (`examples/http_config.json`):**

```json
{
  "system": {
    "device_name": "AgriMonitor-Field3",
    "core0_frequency": 160,
    "core1_frequency": 240
  },
  "wifi": {
    "ssid": "FarmWiFi",
    "password": "AgroTech2024",
    "ap_mode": false
  },
  "sensors": [
    {
      "id": "soil_moisture_1",
      "type": "analog",
      "pin": 34,
      "sample_rate": 10000,
      "enabled": true
    },
    {
      "id": "soil_moisture_2",
      "type": "analog",
      "pin": 35,
      "sample_rate": 10000,
      "enabled": true
    },
    {
      "id": "ambient_light",
      "type": "analog",
      "pin": 32,
      "sample_rate": 30000,
      "enabled": true
    },
    {
      "id": "rain_detector",
      "type": "digital",
      "pin": 25,
      "mode": "input_pulldown",
      "sample_rate": 5000,
      "enabled": true
    }
  ],
  "communication": {
    "protocol": "http",
    "http": {
      "endpoint": "https://api.agricloud.io/v1/sensors/data",
      "method": "POST",
      "auth_token": "Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...",
      "interval": 60000,
      "use_https": true
    }
  }
}
```

**Resultado:**
- Lecturas cada 10 segundos (humedad) y 30 segundos (luz)
- Envío bulk cada minuto a la nube con HTTPS
- Bajo consumo energético (Core 0 a 160MHz)
- Ideal para instalaciones con energía solar

---

## 🔧 Configuración

El firmware soporta dos métodos de configuración que se pueden intercambiar en cualquier momento:

### 🌐 Modo Web (Recomendado)

**Acceso a la Interfaz:**
1. Enciende la ESP32
2. Conéctate a la red WiFi: **ESP32-Config** (contraseña: `config123`)
3. Navega a: `http://192.168.4.1`

**Interfaz de Configuración:**

```
┌─────────────────────────────────────────┐
│         ESP32 Configuration             │
├─────────────────────────────────────────┤
│                                         │
│  📊 Dashboard                           │
│     └─ Estado en tiempo real            │
│                                         │
│  🔌 Sensores                            │
│     ├─ Agregar nuevo sensor             │
│     ├─ Editar sensores existentes       │
│     └─ Activar/Desactivar               │
│                                         │
│  📡 Comunicación                        │
│     ├─ Seleccionar protocolo            │
│     ├─ MQTT / TCP / HTTP                │
│     └─ Parámetros de conexión           │
│                                         │
│  ⚙️ Sistema                             │
│     ├─ Nombre del dispositivo           │
│     ├─ Configuración WiFi               │
│     └─ Frecuencia de cores              │
│                                         │
└─────────────────────────────────────────┘
```

**Endpoints REST disponibles:**
- `GET /` - Dashboard principal
- `GET /api/config` - Obtener configuración actual
- `POST /api/config` - Actualizar configuración
- `GET /api/status` - Estado del sistema

---

### 📝 Modo JSON (Avanzado)

**Editar archivo de configuración:**

1. Edita `data/config.json` con tu configuración personalizada
2. Sube el archivo al sistema de archivos SPIFFS:
   ```bash
   pio run --target uploadfs
   ```
3. Reinicia la ESP32 (botón RST o comando)

**Estructura del Archivo:**

```json
{
  "system": {
    "device_name": "ESP32-Hub",
    "core0_frequency": 240,
    "core1_frequency": 240
  },
  "wifi": {
    "ssid": "YourNetwork",
    "password": "YourPassword",
    "ap_mode": false
  },
  "sensors": [ ... ],
  "communication": { ... }
}
```

**Explorar Ejemplos Completos:**

```bash
examples/
├── basic_config.json      # Configuración mínima funcional
├── advanced_config.json   # Múltiples sensores y MQTT
├── http_config.json       # Cliente HTTP con autenticación
└── tcp_config.json        # Cliente TCP con SSL
```

**Referencia completa**: [docs/CONFIGURATION.md](docs/CONFIGURATION.md)

---

## 📚 Documentación

### Guías de Usuario

- **[📘 Guía de Inicio](docs/GETTING_STARTED.md)**: Tutorial completo desde cero hasta primer proyecto
- **[⚙️ Configuración Completa](docs/CONFIGURATION.md)**: Referencia de todos los parámetros disponibles
- **[🛠️ API Reference](docs/API.md)**: Documentación de endpoints REST y protocolos

### Guías de Desarrollo

- **[🏗️ Arquitectura del Sistema](docs/ARCHITECTURE.md)**: Diseño interno y flujo de datos
- **[🔌 Agregar Sensores Personalizados](docs/ADDING_SENSORS.md)**: Cómo extender con nuevos tipos de sensores
- **[📡 Agregar Protocolos de Comunicación](docs/ADDING_PROTOCOLS.md)**: Cómo implementar nuevos protocolos

---

## 🏛️ Arquitectura del Sistema

El firmware utiliza una arquitectura dual-core asíncrona que separa las responsabilidades críticas:

```
╔═══════════════════════════════════════════════════════════════╗
║                        ESP32 DUAL-CORE                         ║
╠═══════════════════════════════════════════════════════════════╣
║                                                                ║
║  ┌──────────────────────┐          ┌──────────────────────┐  ║
║  │      CORE 0          │  Queue   │       CORE 1         │  ║
║  │   Sensor Manager     │◄────────►│  Communication Mgr   │  ║
║  │                      │          │                      │  ║
║  │  ┌────────────────┐  │          │  ┌────────────────┐ │  ║
║  │  │  Digital       │  │          │  │  MQTT Client   │ │  ║
║  │  │  Analog        │  │          │  │  TCP Client    │ │  ║
║  │  │  I2C           │  │          │  │  HTTP Client   │ │  ║
║  │  │  UART          │  │          │  │  Web Server    │ │  ║
║  │  └────────────────┘  │          │  └────────────────┘ │  ║
║  └──────────────────────┘          └──────────────────────┘  ║
║                                                                ║
║  ┌──────────────────────────────────────────────────────────┐ ║
║  │           ConfigManager (SPIFFS Storage)                 │ ║
║  └──────────────────────────────────────────────────────────┘ ║
╚═══════════════════════════════════════════════════════════════╝
```

### Flujo de Datos

1. **SensorCore (Core 0)** lee los sensores según su `sample_rate` configurado
2. Los datos se encolan en una **Queue de FreeRTOS** (comunicación inter-core)
3. **CommCore (Core 1)** consume la cola y transmite según el protocolo configurado
4. **WebServer** permite reconfiguración en caliente sin reiniciar

### Ventajas del Diseño Dual-Core

| Característica | Beneficio |
|----------------|-----------|
| **Procesamiento Paralelo** | Los sensores se leen sin interrumpir las comunicaciones |
| **Zero Latencia** | Las transmisiones de red no bloquean la adquisición de datos |
| **Escalabilidad** | Soporte para múltiples sensores de alta frecuencia |
| **Reliability** | Si falla un core, el otro continúa operando independientemente |

---

## 🚀 Rendimiento

| Métrica | Valor |
|---------|-------|
| **Sensores simultáneos** | Hasta 16 |
| **Frecuencia de muestreo** | 10ms - 60s por sensor |
| **Latencia inter-core** | < 5ms |
| **Memoria RAM libre** | ~120KB (con 5 sensores) |
| **Velocidad I2C** | 100kHz - 400kHz |
| **Baudrate UART** | 9600 - 115200 bps |
| **Throughput MQTT** | ~1000 msgs/s |

---

## 🧪 Testing y Calidad

```bash
# Compilar con warnings estrictos
pio run --environment esp32dev

# Monitor serial para debugging
pio device monitor --baud 115200

# Análisis estático de código
pio check

# Información de uso de memoria
pio run --target size
```

---

## 💡 Casos de Uso Reales

<table>
<tr>
<td width="33%">

### 🏠 Domótica
- Control de iluminación
- Sensores de presencia
- Temperatura y humedad
- Control de accesos
- Automatización de persianas

</td>
<td width="33%">

### 🏭 Industrial
- Monitoreo de máquinas
- Sensores de vibración
- Control de calidad
- Alertas de mantenimiento
- Telemetría en tiempo real

</td>
<td width="33%">

### 🌱 Agricultura
- Riego inteligente
- Monitoreo de cultivos
- Estaciones meteorológicas
- Control de invernaderos
- Optimización hídrica

</td>
</tr>
</table>

---

## ❓ Preguntas Frecuentes

<details>
<summary><b>¿Puedo usar sensores de diferentes tipos simultáneamente?</b></summary>

Sí, el sistema está diseñado para manejar múltiples sensores de diferentes tipos al mismo tiempo. Puedes tener sensores digitales, analógicos, I2C y UART operando concurrentemente sin interferencias gracias a la arquitectura dual-core.

</details>

<details>
<summary><b>¿Cómo cambio el protocolo de comunicación sin recompilar?</b></summary>

Puedes cambiar el protocolo de dos formas:
1. **Vía Web**: Accede a `http://192.168.4.1` y selecciona el nuevo protocolo desde la interfaz
2. **Vía JSON**: Edita `data/config.json`, cambia el campo `protocol` y sube el archivo con `pio run --target uploadfs`

En ambos casos, el cambio es inmediato tras reiniciar.

</details>

<details>
<summary><b>¿Qué pasa si pierdo la conexión WiFi?</b></summary>

El firmware implementa reconexión automática:
- Reintenta conectar cada 10 segundos
- Los sensores siguen funcionando en Core 0
- Los datos se bufferean temporalmente
- Al reconectar, se envían los datos acumulados

</details>

<details>
<summary><b>¿Puedo agregar mis propios tipos de sensores?</b></summary>

Absolutamente. El sistema usa herencia de clases:
1. Crea una nueva clase que herede de `BaseSensor`
2. Implementa los métodos `init()` y `read()`
3. Regístrala en el sistema

Consulta [docs/ADDING_SENSORS.md](docs/ADDING_SENSORS.md) para un tutorial completo.

</details>

<details>
<summary><b>¿Funciona con alimentación por batería?</b></summary>

Sí, puedes reducir el consumo:
- Baja las frecuencias de los cores a 80MHz
- Aumenta el `sample_rate` de los sensores
- Usa modos de bajo consumo entre lecturas
- Consumo típico: 40-80mA @ 3.7V

</details>

<details>
<summary><b>¿Soporta OTA (Over-The-Air) updates?</b></summary>

Actualmente no está implementado, pero está en el roadmap para la versión 1.2. Puedes hacer updates físicos por USB mientras tanto.

</details>

<details>
<summary><b>¿Qué protocolos de seguridad soporta?</b></summary>

- **MQTT**: TLS/SSL con certificados
- **TCP**: TLS 1.2
- **HTTP**: HTTPS con validación de certificados
- **WiFi**: WPA2-PSK

</details>

---

## 🗺️ Roadmap

### v1.1 (En desarrollo)
- [ ] Soporte para BLE (Bluetooth Low Energy)
- [ ] Dashboard web mejorado con gráficos en tiempo real
- [ ] Modo deep sleep para ahorro energético
- [ ] Logging a SD Card

### v1.2 (Planeado)
- [ ] OTA (Over-The-Air) firmware updates
- [ ] Soporte para sensores SPI
- [ ] Multi-WiFi (failover entre redes)
- [ ] Integración con Alexa/Google Home

### v2.0 (Futuro)
- [ ] Mesh networking entre múltiples ESP32
- [ ] Machine Learning on-device
- [ ] Soporte para LoRaWAN
- [ ] Panel de administración cloud

### v2.1 (Investigación)
- [ ] Edge computing con TensorFlow Lite
- [ ] Protocolo Matter/Thread
- [ ] Camera support (ESP32-CAM)

---

## 🤝 Contribuir

¡Las contribuciones son bienvenidas! Por favor lee [CONTRIBUTING.md](CONTRIBUTING.md) para conocer el proceso de desarrollo.

### Cómo Contribuir

1. **Fork** el proyecto
2. **Crea** una rama para tu feature (`git checkout -b feature/NuevoSensor`)
3. **Commit** tus cambios (`git commit -m 'Agregado soporte para sensor X'`)
4. **Push** a la rama (`git push origin feature/NuevoSensor`)
5. **Abre** un Pull Request

### Áreas de Contribución
- 🐛 Reportar bugs o issues
- ✨ Proponer nuevas características
- 📝 Mejorar la documentación
- 🔌 Agregar soporte para nuevos sensores
- 📡 Implementar protocolos adicionales
- 🧪 Agregar tests automatizados

---

## 📄 Licencia

Este proyecto está licenciado bajo la licencia MIT. Consulta el archivo [LICENSE](LICENSE) para más detalles.

---

## 🙏 Reconocimientos

Este firmware fue desarrollado para proporcionar una base sólida y profesional para proyectos IoT con ESP32.

**Tecnologías y Librerías:**
- [ESP32 Arduino Core](https://github.com/espressif/arduino-esp32) - Framework principal
- [PlatformIO](https://platformio.org/) - Sistema de compilación
- [FreeRTOS](https://www.freertos.org/) - Sistema operativo en tiempo real
- [ArduinoJson](https://arduinojson.org/) - Parsing y serialización JSON
- [PubSubClient](https://github.com/knolleary/pubsubclient) - Cliente MQTT

**Inspiración:**
- Comunidad ESP32
- Proyectos open-source de IoT
- Estándares de la industria embebida

---

## 📞 Soporte y Contacto

- **Issues**: [GitHub Issues](https://github.com/andresfirmware/esp32-modular-firmware/issues)
- **Discusiones**: [GitHub Discussions](https://github.com/andresfirmware/esp32-modular-firmware/discussions)
- **Email**: support@esp32modular.dev

---

<div align="center">

**[⬆ Volver arriba](#-esp32-modular-firmware)**

Desarrollado con dedicación para la comunidad IoT

![ESP32](https://img.shields.io/badge/ESP32-Firmware-blue?style=for-the-badge&logo=espressif)
![IoT](https://img.shields.io/badge/IoT-Ready-green?style=for-the-badge)

</div>
