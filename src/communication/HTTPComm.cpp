#include "HTTPComm.h"

HTTPComm::HTTPComm(const HTTPCredentials& creds) 
    : credentials(creds) {
}

HTTPComm::~HTTPComm() {
    disconnect();
}

bool HTTPComm::connect() {
    // HTTP no requiere conexión persistente, solo validamos la configuración
    if (credentials.endpoint.length() == 0) {
        Serial.println("[HTTP] Error: Endpoint no configurado");
        return false;
    }
    
    Serial.println("[HTTP] Configuración validada");
    Serial.printf("  Endpoint: %s\n", credentials.endpoint.c_str());
    Serial.printf("  Método: %s\n", credentials.method.c_str());
    
    connected = true;
    return true;
}

bool HTTPComm::disconnect() {
    http.end();
    connected = false;
    return true;
}

bool HTTPComm::send(const String& topic, const String& payload) {
    if (!connected) {
        return false;
    }
    
    // Construir JSON con topic y payload
    String json = "{\"topic\":\"" + topic + "\",\"data\":" + payload + "}";
    
    return sendJSON(json);
}

void HTTPComm::loop() {
    // HTTP is stateless - no active loop required
    // Future: implement periodic buffer flush
}

bool HTTPComm::sendJSON(const String& json) {
    http.begin(credentials.endpoint);
    http.addHeader("Content-Type", "application/json");
    
    if (credentials.authToken.length() > 0) {
        http.addHeader("Authorization", "Bearer " + credentials.authToken);
    }
    
    int httpCode;
    
    if (credentials.method == "POST") {
        httpCode = http.POST(json);
    } else if (credentials.method == "PUT") {
        httpCode = http.PUT(json);
    } else {
        // GET con parámetros en URL
        String url = credentials.endpoint + "?data=" + json;
        http.begin(url);
        httpCode = http.GET();
    }
    
    bool success = (httpCode == 200 || httpCode == 201);
    
    if (success) {
        Serial.printf("[HTTP] Enviado exitosamente (Código: %d)\n", httpCode);
        String response = http.getString();
        Serial.printf("[HTTP] Respuesta: %s\n", response.c_str());
    } else {
        Serial.printf("[HTTP] Error en envío (Código: %d)\n", httpCode);
    }
    
    http.end();
    return success;
}

bool HTTPComm::sendFormData(const String& data) {
    http.begin(credentials.endpoint);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    
    if (credentials.authToken.length() > 0) {
        http.addHeader("Authorization", "Bearer " + credentials.authToken);
    }
    
    int httpCode = http.POST(data);
    bool success = (httpCode == 200 || httpCode == 201);
    
    http.end();
    return success;
}

void HTTPComm::addHeader(const String& name, const String& value) {
    http.addHeader(name, value);
}
