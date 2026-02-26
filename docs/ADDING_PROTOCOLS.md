# Agregar Protocolos de Comunicación

Esta guía explica cómo agregar soporte para nuevos protocolos de comunicación al firmware.

## Estructura de un Protocolo

Todos los protocolos deben heredar de la clase base `BaseComm`:

```cpp
class BaseComm {
public:
    virtual bool connect() = 0;
    virtual bool disconnect() = 0;
    virtual bool send(const String& topic, const String& payload) = 0;
    virtual void loop() = 0;
    virtual String getType() = 0;
    
    // Método común ya implementado
    bool isConnected() const;
};
```

## Paso 1: Crear la Clase del Protocolo

Crea dos archivos en `src/communication/`:
- `MyProtocol.h` (header)
- `MyProtocol.cpp` (implementación)

### Ejemplo: WebSocket

**src/communication/WebSocketComm.h**
```cpp
#ifndef WEBSOCKET_COMM_H
#define WEBSOCKET_COMM_H

#include "BaseComm.h"
#include <WiFi.h>
#include <WebSocketsClient.h>

struct WebSocketCredentials {
    String host;
    uint16_t port;
    String path;
    String protocol;
};

class WebSocketComm : public BaseComm {
public:
    WebSocketComm(const WebSocketCredentials& credentials);
    ~WebSocketComm();
    
    bool connect() override;
    bool disconnect() override;
    bool send(const String& topic, const String& payload) override;
    void loop() override;
    String getType() override { return "websocket"; }
    
    // Métodos específicos WebSocket
    void setCallback(std::function<void(String)> callback);
    bool sendText(const String& message);
    bool sendBinary(uint8_t* data, size_t length);
    
private:
    WebSocketCredentials credentials;
    WebSocketsClient* wsClient;
    
    std::function<void(String)> messageCallback;
    
    static void webSocketEvent(WStype_t type, uint8_t* payload, size_t length);
    static WebSocketComm* instance;
};

#endif
```

**src/communication/WebSocketComm.cpp**
```cpp
#include "WebSocketComm.h"

WebSocketComm* WebSocketComm::instance = nullptr;

WebSocketComm::WebSocketComm(const WebSocketCredentials& creds) 
    : credentials(creds) {
    
    wsClient = new WebSocketsClient();
    instance = this;
}

WebSocketComm::~WebSocketComm() {
    disconnect();
    delete wsClient;
}

bool WebSocketComm::connect() {
    Serial.println("[WebSocket] Intentando conectar...");
    Serial.printf("  Host: %s:%d%s\n", 
                 credentials.host.c_str(), 
                 credentials.port, 
                 credentials.path.c_str());
    
    wsClient->begin(
        credentials.host.c_str(), 
        credentials.port, 
        credentials.path.c_str(),
        credentials.protocol.c_str()
    );
    
    wsClient->onEvent(webSocketEvent);
    wsClient->setReconnectInterval(5000);
    
    // WebSocket se conecta de forma asíncrona
    connected = false;
    
    return true;
}

bool WebSocketComm::disconnect() {
    wsClient->disconnect();
    connected = false;
    Serial.println("[WebSocket] Desconectado");
    return true;
}

bool WebSocketComm::send(const String& topic, const String& payload) {
    if (!connected) {
        return false;
    }
    
    // Formato JSON para envío
    String message = "{\"topic\":\"" + topic + "\",\"data\":" + payload + "}";
    
    return sendText(message);
}

void WebSocketComm::loop() {
    wsClient->loop();
}

bool WebSocketComm::sendText(const String& message) {
    if (!connected) return false;
    
    bool result = wsClient->sendTXT(message);
    
    if (result) {
        Serial.printf("[WebSocket] Enviado: %s\n", message.c_str());
    } else {
        Serial.println("[WebSocket] Error enviando mensaje");
    }
    
    return result;
}

bool WebSocketComm::sendBinary(uint8_t* data, size_t length) {
    if (!connected) return false;
    return wsClient->sendBIN(data, length);
}

void WebSocketComm::setCallback(std::function<void(String)> callback) {
    messageCallback = callback;
}

void WebSocketComm::webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
    if (instance == nullptr) return;
    
    switch(type) {
        case WStype_DISCONNECTED:
            Serial.println("[WebSocket] Desconectado");
            instance->connected = false;
            break;
            
        case WStype_CONNECTED:
            Serial.println("[WebSocket] Conectado");
            instance->connected = true;
            break;
            
        case WStype_TEXT:
            Serial.printf("[WebSocket] Mensaje recibido: %s\n", payload);
            if (instance->messageCallback) {
                instance->messageCallback(String((char*)payload));
            }
            break;
            
        case WStype_BIN:
            Serial.printf("[WebSocket] Datos binarios recibidos: %u bytes\n", length);
            break;
            
        case WStype_ERROR:
            Serial.println("[WebSocket] Error");
            break;
            
        case WStype_PING:
        case WStype_PONG:
            // Handled automatically
            break;
    }
}
```

## Paso 2: Actualizar ConfigManager

Agrega la estructura de configuración en `src/config/ConfigManager.h`:

```cpp
struct WebSocketConfig {
    String host;
    uint16_t port;
    String path;
    String protocol;
};

struct CommunicationConfig {
    String protocol;
    MQTTConfig mqtt;
    TCPConfig tcp;
    HTTPConfig http;
    WebSocketConfig websocket;  // Nuevo
};
```

Y el parsing en `ConfigManager.cpp`:

```cpp
bool ConfigManager::parseJson(const JsonDocument& doc) {
    // ... código existente ...
    
    if (doc["communication"].containsKey("websocket")) {
        commConfig.websocket.host = doc["communication"]["websocket"]["host"] | "";
        commConfig.websocket.port = doc["communication"]["websocket"]["port"] | 80;
        commConfig.websocket.path = doc["communication"]["websocket"]["path"] | "/";
        commConfig.websocket.protocol = doc["communication"]["websocket"]["protocol"] | "";
    }
    
    return true;
}
```

## Paso 3: Integrar con CommCore

Modifica `src/core/CommCore.cpp`:

```cpp
#include "WebSocketComm.h"

void CommCore::loadCommunication(ConfigManager& config) {
    // ... código existente ...
    
    else if (commConfig.protocol == "websocket") {
        Serial.println("  - Host WebSocket: " + commConfig.websocket.host);
        Serial.printf("  - Puerto: %d\n", commConfig.websocket.port);
        
        WebSocketCredentials creds;
        creds.host = commConfig.websocket.host;
        creds.port = commConfig.websocket.port;
        creds.path = commConfig.websocket.path;
        creds.protocol = commConfig.websocket.protocol;
        
        commProtocol = new WebSocketComm(creds);
        
        if (commProtocol != nullptr) {
            commProtocol->connect();
        }
    }
}
```

## Paso 4: Actualizar platformio.ini

Agrega las librerías necesarias:

```ini
lib_deps = 
    ...existing deps...
    links2004/WebSockets@^2.3.6
```

## Ejemplo Avanzado: CoAP (Constrained Application Protocol)

Para protocolos más especializados:

**src/communication/CoAPComm.h**
```cpp
#ifndef COAP_COMM_H
#define COAP_COMM_H

#include "BaseComm.h"
#include <coap-simple.h>

struct CoAPCredentials {
    String server;
    uint16_t port;
};

class CoAPComm : public BaseComm {
public:
    CoAPComm(const CoAPCredentials& credentials);
    ~CoAPComm();
    
    bool connect() override;
    bool disconnect() override;
    bool send(const String& topic, const String& payload) override;
    void loop() override;
    String getType() override { return "coap"; }
    
private:
    CoAPCredentials credentials;
    Coap* coap;
    WiFiUDP udp;
    
    static void response(CoapPacket& packet, IPAddress ip, int port);
};

#endif
```

## Ejemplo: LoRaWAN

Para comunicación de largo alcance:

**src/communication/LoRaWANComm.h**
```cpp
#ifndef LORAWAN_COMM_H
#define LORAWAN_COMM_H

#include "BaseComm.h"
#include <lmic.h>

struct LoRaWANCredentials {
    uint8_t devEUI[8];
    uint8_t appEUI[8];
    uint8_t appKey[16];
};

class LoRaWANComm : public BaseComm {
public:
    LoRaWANComm(const LoRaWANCredentials& credentials);
    
    bool connect() override;
    bool disconnect() override;
    bool send(const String& topic, const String& payload) override;
    void loop() override;
    String getType() override { return "lorawan"; }
    
private:
    LoRaWANCredentials credentials;
    bool joined;
    
    void onEvent(ev_t ev);
};

#endif
```

## Configuración JSON

**WebSocket:**
```json
{
  "communication": {
    "protocol": "websocket",
    "websocket": {
      "host": "ws.example.com",
      "port": 8080,
      "path": "/sensors",
      "protocol": ""
    }
  }
}
```

**CoAP:**
```json
{
  "communication": {
    "protocol": "coap",
    "coap": {
      "server": "coap.example.com",
      "port": 5683
    }
  }
}
```

**LoRaWAN:**
```json
{
  "communication": {
    "protocol": "lorawan",
    "lorawan": {
      "devEUI": "0000000000000000",
      "appEUI": "0000000000000000",
      "appKey": "00000000000000000000000000000000"
    }
  }
}
```

## Checklist de Integración

- [ ] Crear clase heredando de `BaseComm`
- [ ] Implementar métodos: `connect()`, `disconnect()`, `send()`, `loop()`, `getType()`
- [ ] Agregar estructura de configuración en `ConfigManager.h`
- [ ] Implementar parsing en `ConfigManager.cpp`
- [ ] Integrar en `CommCore::loadCommunication()`
- [ ] Agregar dependencias en `platformio.ini`
- [ ] Documentar configuración JSON
- [ ] Crear ejemplo en `examples/`
- [ ] Probar conectividad
- [ ] Actualizar README

## Tips

1. **Reconexión**: Implementa lógica de reconexión automática
2. **Buffer**: Considera usar buffers para envíos cuando esté desconectado
3. **Timeouts**: Implementa timeouts apropiados
4. **Callbacks**: Usa callbacks para eventos asíncronos
5. **Seguridad**: Implementa TLS/SSL cuando sea apropiado
6. **Rate limiting**: Respeta límites de velocidad del servicio
7. **Logging**: Registra todos los eventos importantes

## Protocolos Comunes

### MQTT-SN (para redes con restricciones)
- Versión ligera de MQTT
- Ideal para redes zigbee/6LoWPAN

### Bluetooth Low Energy (BLE)
- Para comunicación de corto alcance
- Bajo consumo energético

### Modbus (industrial)
- RTU o TCP
- Común en aplicaciones industriales

### OPC UA
- Protocolo industrial moderno
- Seguridad incorporada

¿Implementaste un nuevo protocolo? ¡Compártelo con la comunidad!
