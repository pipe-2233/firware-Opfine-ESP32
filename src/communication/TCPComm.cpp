#include "TCPComm.h"

TCPComm::TCPComm(const TCPCredentials& creds) 
    : credentials(creds), lastReconnectAttempt(0), reconnectDelay(5000) {
    
    client = new WiFiClient();
}

TCPComm::~TCPComm() {
    disconnect();
    delete client;
}

bool TCPComm::connect() {
    Serial.println("[TCP] Intentando conectar...");
    Serial.printf("  Servidor: %s:%d\n", credentials.server.c_str(), credentials.port);
    
    if (client->connect(credentials.server.c_str(), credentials.port)) {
        Serial.println("[TCP] Conectado exitosamente");
        connected = true;
        return true;
    } else {
        Serial.println("[TCP] Error de conexión");
        connected = false;
        return false;
    }
}

bool TCPComm::disconnect() {
    if (client->connected()) {
        client->stop();
    }
    connected = false;
    Serial.println("[TCP] Desconectado");
    return true;
}

bool TCPComm::send(const String& topic, const String& payload) {
    if (!connected) {
        return false;
    }
    
    // Formato: [topic]|[payload]\n
    String message = topic + "|" + payload + "\n";
    
    size_t written = client->print(message);
    
    if (written > 0) {
        Serial.printf("[TCP] Enviado: %s\n", message.c_str());
        return true;
    } else {
        Serial.println("[TCP] Error enviando datos");
        return false;
    }
}

void TCPComm::loop() {
    if (!client->connected()) {
        connected = false;
        
        unsigned long now = millis();
        if (now - lastReconnectAttempt > reconnectDelay) {
            lastReconnectAttempt = now;
            if (reconnect()) {
                lastReconnectAttempt = 0;
            }
        }
    } else {
        // Procesar datos recibidos si los hay
        while (client->available()) {
            String data = receive();
            if (data.length() > 0) {
                Serial.printf("[TCP] Recibido: %s\n", data.c_str());
            }
        }
    }
}

bool TCPComm::reconnect() {
    Serial.println("[TCP] Intentando reconectar...");
    return connect();
}

bool TCPComm::sendRaw(const uint8_t* data, size_t length) {
    if (!connected) return false;
    
    size_t written = client->write(data, length);
    return written == length;
}

int TCPComm::available() {
    if (!connected) return 0;
    return client->available();
}

String TCPComm::receive() {
    if (!connected || !client->available()) {
        return "";
    }
    
    String data = "";
    while (client->available()) {
        char c = client->read();
        if (c == '\n') break;
        data += c;
    }
    
    return data;
}
