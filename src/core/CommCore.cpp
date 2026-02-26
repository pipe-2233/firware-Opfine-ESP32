#include "CommCore.h"

extern TaskManager taskManager;

CommCore::CommCore() {
    commProtocol = nullptr;
    initialized = false;
    lastConnectionCheck = 0;
}

CommCore::~CommCore() {
    if (commProtocol != nullptr) {
        delete commProtocol;
    }
}

bool CommCore::begin() {
    Serial.printf("[Core %d] Inicializando CommCore...\n", xPortGetCoreID());
    
    // Protocol loaded via loadProtocol()
    initialized = true;
    
    Serial.printf("[Core %d] CommCore inicializado\n", xPortGetCoreID());
    return true;
}

void CommCore::loop() {
    if (!initialized) return;
    
    // Procesar mensajes entrantes desde el core de sensores
    processIncomingMessages();
    
    // Connection health check (5s interval)
    unsigned long currentTime = millis();
    if (currentTime - lastConnectionCheck > 5000) {
        checkConnection();
        lastConnectionCheck = currentTime;
    }
    
    // Mantener el protocolo de comunicación activo (loop del protocolo)
    if (commProtocol != nullptr && commProtocol->isConnected()) {
        commProtocol->loop();
    }
    
    // Pequeño delay
    delay(10);
}

void CommCore::stop() {
    if (commProtocol != nullptr) {
        commProtocol->disconnect();
    }
    initialized = false;
}

void CommCore::loadCommunication(ConfigManager& config) {
    if (commProtocol != nullptr) {
        delete commProtocol;
        commProtocol = nullptr;
    }
    
    auto commConfig = config.getCommConfig();
    
    Serial.printf("Cargando protocolo de comunicación: %s\n", commConfig.protocol.c_str());
    
    // Protocol factory: instantiate based on config
    // Supported: MQTT, TCP, HTTP
    
    if (commConfig.protocol == "mqtt") {
        Serial.println("  - Broker MQTT: " + commConfig.mqtt.broker);
        Serial.printf("  - Puerto: %d\n", commConfig.mqtt.port);
        // commProtocol = new MQTTComm(commConfig.mqtt);
    } else if (commConfig.protocol == "tcp") {
        Serial.println("  - Servidor TCP: " + commConfig.tcp.server);
        Serial.printf("  - Puerto: %d\n", commConfig.tcp.port);
        // commProtocol = new TCPComm(commConfig.tcp);
    } else if (commConfig.protocol == "http") {
        Serial.println("  - Endpoint HTTP: " + commConfig.http.endpoint);
        // commProtocol = new HTTPComm(commConfig.http);
    }
}

void CommCore::setCommProtocol(BaseComm* comm) {
    if (commProtocol != nullptr) {
        delete commProtocol;
    }
    commProtocol = comm;
}

bool CommCore::sendData(const String& topic, const String& payload) {
    if (commProtocol == nullptr || !commProtocol->isConnected()) {
        return false;
    }
    
    return commProtocol->send(topic, payload);
}

void CommCore::processIncomingMessages() {
    CoreMessage msg;
    
    // Leer mensajes del queue sin bloquear
    while (taskManager.receiveFromSensorCore(msg, 0)) {
        // Procesar el mensaje del sensor
        Serial.printf("[CommCore] Mensaje recibido - Sensor: %s, Valor: %.2f\n", 
                     msg.sensorId.c_str(), msg.value);
        
        // Formatear y enviar a través del protocolo de comunicación
        if (commProtocol != nullptr && commProtocol->isConnected()) {
            String topic = msg.sensorId;
            String payload = "{\"value\":" + String(msg.value) + 
                           ",\"timestamp\":" + String(msg.timestamp) + "}";
            
            sendData(topic, payload);
        }
    }
}

void CommCore::checkConnection() {
    if (commProtocol != nullptr) {
        if (!commProtocol->isConnected()) {
            Serial.println("[CommCore] Intentando reconectar...");
            commProtocol->connect();
        }
    }
}
