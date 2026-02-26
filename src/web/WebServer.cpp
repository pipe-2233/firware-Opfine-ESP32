#include "WebServer.h"

ConfigWebServer::ConfigWebServer(ConfigManager& config) 
    : configManager(config), server(nullptr) {
    server = new WebServer(80);
}

bool ConfigWebServer::begin() {
    Serial.println("[WebServer] Iniciando servidor web...");
    
    // Configurar rutas
    server->on("/", [this]() { handleRoot(); });
    server->on("/api/config", HTTP_GET, [this]() { handleGetConfig(); });
    server->on("/api/config", HTTP_POST, [this]() { handleSaveConfig(); });
    server->on("/sensors", [this]() { handleSensors(); });
    server->on("/api/sensor/add", HTTP_POST, [this]() { handleAddSensor(); });
    server->on("/api/sensor/remove", HTTP_POST, [this]() { handleRemoveSensor(); });
    server->on("/communication", [this]() { handleCommunication(); });
    server->on("/api/restart", HTTP_POST, [this]() { handleRestart(); });
    server->onNotFound([this]() { handleNotFound(); });
    
    server->begin();
    
    Serial.println("[WebServer] Servidor iniciado en puerto 80");
    return true;
}

void ConfigWebServer::loop() {
    server->handleClient();
}

void ConfigWebServer::stop() {
    server->stop();
}

void ConfigWebServer::handleRoot() {
    String html = getHTMLHeader();
    html += "<h1>ESP32 Firmware Modular</h1>";
    html += "<div class='card'>";
    html += "<h2>Configuración del Dispositivo</h2>";
    html += "<p>Nombre: <strong>" + configManager.getSystemConfig().device_name + "</strong></p>";
    html += "<div class='menu'>";
    html += "<a href='/sensors'>Configurar Sensores</a>";
    html += "<a href='/communication'>Configurar Comunicación</a>";
    html += "<a href='/api/config'>Ver JSON</a>";
    html += "</div>";
    html += "</div>";
    html += getHTMLFooter();
    
    server->send(200, "text/html", html);
}

void ConfigWebServer::handleGetConfig() {
    String json = configManager.toJson();
    server->send(200, "application/json", json);
}

void ConfigWebServer::handleSaveConfig() {
    if (server->hasArg("plain")) {
        String json = server->arg("plain");
        
        if (configManager.loadFromJson(json)) {
            configManager.saveConfig();
            server->send(200, "application/json", "{\"success\":true,\"message\":\"Configuración guardada\"}");
        } else {
            server->send(400, "application/json", "{\"success\":false,\"message\":\"Error en JSON\"}");
        }
    } else {
        server->send(400, "application/json", "{\"success\":false,\"message\":\"No se recibió JSON\"}");
    }
}

void ConfigWebServer::handleSensors() {
    String html = generateSensorsPage();
    server->send(200, "text/html", html);
}

void ConfigWebServer::handleAddSensor() {
    // POST /api/sensor - Crear nuevo sensor desde web UI
    String id = server->arg("id");
    String type = server->arg("type");
    uint8_t pin = server->arg("pin").toInt();
    
    SensorConfig sensor;
    sensor.id = id;
    sensor.type = type;
    sensor.pin = pin;
    sensor.enabled = true;
    sensor.sample_rate = 1000;
    
    configManager.addSensor(sensor);
    configManager.saveConfig();
    
    server->send(200, "application/json", "{\"success\":true}");
}

void ConfigWebServer::handleRemoveSensor() {
    String id = server->arg("id");
    configManager.removeSensor(id);
    configManager.saveConfig();
    
    server->send(200, "application/json", "{\"success\":true}");
}

void ConfigWebServer::handleCommunication() {
    String html = generateCommPage();
    server->send(200, "text/html", html);
}

void ConfigWebServer::handleRestart() {
    server->send(200, "application/json", "{\"success\":true,\"message\":\"Reiniciando...\"}");
    delay(1000);
    ESP.restart();
}

void ConfigWebServer::handleNotFound() {
    server->send(404, "text/plain", "404: Página no encontrada");
}

String ConfigWebServer::getHTMLHeader() {
    return R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset='UTF-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <title>ESP32 Config</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            max-width: 800px;
            margin: 0 auto;
            padding: 20px;
            background-color: #f5f5f5;
        }
        h1 {
            color: #333;
            text-align: center;
        }
        h2 {
            color: #555;
        }
        .card {
            background: white;
            border-radius: 8px;
            padding: 20px;
            margin: 20px 0;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
        }
        .menu {
            display: flex;
            flex-direction: column;
            gap: 10px;
            margin-top: 20px;
        }
        .menu a, button {
            background: #007bff;
            color: white;
            padding: 12px 20px;
            text-decoration: none;
            border-radius: 4px;
            text-align: center;
            border: none;
            cursor: pointer;
            font-size: 16px;
        }
        .menu a:hover, button:hover {
            background: #0056b3;
        }
        .sensor-item {
            border: 1px solid #ddd;
            padding: 10px;
            margin: 10px 0;
            border-radius: 4px;
        }
        input, select {
            width: 100%;
            padding: 8px;
            margin: 5px 0;
            border: 1px solid #ddd;
            border-radius: 4px;
            box-sizing: border-box;
        }
        label {
            display: block;
            margin-top: 10px;
            font-weight: bold;
        }
    </style>
</head>
<body>
)";
}

String ConfigWebServer::getHTMLFooter() {
    return R"(
</body>
</html>
)";
}

String ConfigWebServer::generateSensorsPage() {
    String html = getHTMLHeader();
    html += "<h1>Configuración de Sensores</h1>";
    html += "<a href='/'>← Volver</a>";
    
    html += "<div class='card'>";
    html += "<h2>Sensores Configurados</h2>";
    
    auto sensors = configManager.getSensors();
    if (sensors.empty()) {
        html += "<p>No hay sensores configurados.</p>";
    } else {
        for (const auto& sensor : sensors) {
            html += "<div class='sensor-item'>";
            html += "<strong>" + sensor.id + "</strong> - ";
            html += "Tipo: " + sensor.type + ", Pin: " + String(sensor.pin);
            html += " <button onclick=\"removeSensor('" + sensor.id + "')\">Eliminar</button>";
            html += "</div>";
        }
    }
    html += "</div>";
    
    html += "<div class='card'>";
    html += "<h2>Agregar Nuevo Sensor</h2>";
    html += "<form id='addSensorForm'>";
    html += "<label>ID del Sensor:</label>";
    html += "<input type='text' id='sensorId' required>";
    html += "<label>Tipo:</label>";
    html += "<select id='sensorType'>";
    html += "<option value='digital'>Digital</option>";
    html += "<option value='analog'>Analógico</option>";
    html += "<option value='i2c'>I2C</option>";
    html += "<option value='uart'>UART</option>";
    html += "</select>";
    html += "<label>Pin:</label>";
    html += "<input type='number' id='sensorPin' required>";
    html += "<button type='submit' style='margin-top: 10px;'>Agregar Sensor</button>";
    html += "</form>";
    html += "</div>";
    
    html += R"(
<script>
document.getElementById('addSensorForm').addEventListener('submit', function(e) {
    e.preventDefault();
    const id = document.getElementById('sensorId').value;
    const type = document.getElementById('sensorType').value;
    const pin = document.getElementById('sensorPin').value;
    
    fetch('/api/sensor/add', {
        method: 'POST',
        headers: {'Content-Type': 'application/x-www-form-urlencoded'},
        body: 'id=' + id + '&type=' + type + '&pin=' + pin
    })
    .then(response => response.json())
    .then(data => {
        if(data.success) {
            location.reload();
        }
    });
});

function removeSensor(id) {
    if(confirm('¿Eliminar sensor ' + id + '?')) {
        fetch('/api/sensor/remove', {
            method: 'POST',
            headers: {'Content-Type': 'application/x-www-form-urlencoded'},
            body: 'id=' + id
        })
        .then(response => response.json())
        .then(data => {
            if(data.success) {
                location.reload();
            }
        });
    }
}
</script>
)";
    
    html += getHTMLFooter();
    return html;
}

String ConfigWebServer::generateCommPage() {
    String html = getHTMLHeader();
    html += "<h1>Configuración de Comunicación</h1>";
    html += "<a href='/'>← Volver</a>";
    
    auto commConfig = configManager.getCommConfig();
    
    html += "<div class='card'>";
    html += "<h2>Protocolo Actual: " + commConfig.protocol + "</h2>";
    html += "<p><em>Usa el editor JSON avanzado o edita el archivo de configuración para cambiar parámetros.</em></p>";
    html += "<a href='/api/config' target='_blank'>Ver Configuración JSON</a>";
    html += "</div>";
    
    html += getHTMLFooter();
    return html;
}
