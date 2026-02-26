#include "TaskManager.h"
#include "SensorCore.h"
#include "CommCore.h"

TaskManager* TaskManager::instance = nullptr;

TaskManager::TaskManager() {
    sensorTaskHandle = nullptr;
    commTaskHandle = nullptr;
    sensorToCommQueue = nullptr;
    commToSensorQueue = nullptr;
    configMutex = nullptr;
    sensorTaskRunning = false;
    commTaskRunning = false;
    instance = this;
}

bool TaskManager::begin() {
    // Crear queues para comunicación entre cores
    sensorToCommQueue = xQueueCreate(20, sizeof(CoreMessage));
    commToSensorQueue = xQueueCreate(10, sizeof(CoreMessage));
    
    if (sensorToCommQueue == nullptr || commToSensorQueue == nullptr) {
        Serial.println("Error creando queues");
        return false;
    }
    
    // Crear mutex para acceso a configuración
    configMutex = xSemaphoreCreateMutex();
    if (configMutex == nullptr) {
        Serial.println("Error creando mutex");
        return false;
    }
    
    Serial.println("TaskManager inicializado");
    return true;
}

void TaskManager::startTasks() {
    // Crear tarea de sensores en Core 0
    xTaskCreatePinnedToCore(
        sensorTaskWrapper,      // Función de la tarea
        "SensorTask",           // Nombre de la tarea
        8192,                   // Tamaño del stack
        this,                   // Parámetro pasado a la tarea
        1,                      // Prioridad
        &sensorTaskHandle,      // Handle de la tarea
        0                       // Core 0
    );
    
    // Crear tarea de comunicación en Core 1
    xTaskCreatePinnedToCore(
        commTaskWrapper,        // Función de la tarea
        "CommTask",             // Nombre de la tarea
        8192,                   // Tamaño del stack
        this,                   // Parámetro pasado a la tarea
        1,                      // Prioridad
        &commTaskHandle,        // Handle de la tarea
        1                       // Core 1
    );
    
    sensorTaskRunning = true;
    commTaskRunning = true;
    
    Serial.println("Tareas iniciadas:");
    Serial.printf("  - SensorTask en Core 0\n");
    Serial.printf("  - CommTask en Core 1\n");
}

void TaskManager::stopTasks() {
    sensorTaskRunning = false;
    commTaskRunning = false;
    
    if (sensorTaskHandle != nullptr) {
        vTaskDelete(sensorTaskHandle);
        sensorTaskHandle = nullptr;
    }
    
    if (commTaskHandle != nullptr) {
        vTaskDelete(commTaskHandle);
        commTaskHandle = nullptr;
    }
    
    Serial.println("Tareas detenidas");
}

bool TaskManager::sendToCommCore(const CoreMessage& msg) {
    if (sensorToCommQueue == nullptr) return false;
    return xQueueSend(sensorToCommQueue, &msg, pdMS_TO_TICKS(100)) == pdTRUE;
}

bool TaskManager::receiveFromSensorCore(CoreMessage& msg, uint32_t timeout) {
    if (sensorToCommQueue == nullptr) return false;
    return xQueueReceive(sensorToCommQueue, &msg, pdMS_TO_TICKS(timeout)) == pdTRUE;
}

void TaskManager::sensorTaskWrapper(void* parameter) {
    TaskManager* manager = static_cast<TaskManager*>(parameter);
    manager->sensorTask();
}

void TaskManager::commTaskWrapper(void* parameter) {
    TaskManager* manager = static_cast<TaskManager*>(parameter);
    manager->commTask();
}

void TaskManager::sensorTask() {
    Serial.printf("[Core %d] SensorTask iniciada\n", xPortGetCoreID());
    
    SensorCore sensorCore;
    sensorCore.begin();
    
    while (sensorTaskRunning) {
        sensorCore.loop();
        vTaskDelay(pdMS_TO_TICKS(10)); // Small delay to prevent watchdog
    }
    
    Serial.println("SensorTask finalizada");
    vTaskDelete(nullptr);
}

void TaskManager::commTask() {
    Serial.printf("[Core %d] CommTask iniciada\n", xPortGetCoreID());
    
    CommCore commCore;
    commCore.begin();
    
    while (commTaskRunning) {
        commCore.loop();
        vTaskDelay(pdMS_TO_TICKS(10)); // Small delay to prevent watchdog
    }
    
    Serial.println("CommTask finalizada");
    vTaskDelete(nullptr);
}
