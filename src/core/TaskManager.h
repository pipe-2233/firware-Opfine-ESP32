#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

// Estructura de mensaje entre cores
struct CoreMessage {
    String sensorId;
    String type;
    float value;
    unsigned long timestamp;
};

class TaskManager {
public:
    TaskManager();
    
    bool begin();
    void startTasks();
    void stopTasks();
    
    // Queue para comunicación entre cores
    bool sendToCommCore(const CoreMessage& msg);
    bool receiveFromSensorCore(CoreMessage& msg, uint32_t timeout = 0);
    
    // Handles de las tareas
    TaskHandle_t getSensorTaskHandle() { return sensorTaskHandle; }
    TaskHandle_t getCommTaskHandle() { return commTaskHandle; }
    
    // Estado de las tareas
    bool isSensorTaskRunning() { return sensorTaskRunning; }
    bool isCommTaskRunning() { return commTaskRunning; }
    
private:
    TaskHandle_t sensorTaskHandle;
    TaskHandle_t commTaskHandle;
    
    QueueHandle_t sensorToCommQueue;
    QueueHandle_t commToSensorQueue;
    
    SemaphoreHandle_t configMutex;
    
    bool sensorTaskRunning;
    bool commTaskRunning;
    
    // Funciones de tarea estáticas (callbacks para FreeRTOS)
    static void sensorTaskWrapper(void* parameter);
    static void commTaskWrapper(void* parameter);
    
    // Funciones de tarea reales
    void sensorTask();
    void commTask();
    
    // Singleton para acceder desde funciones estáticas
    static TaskManager* instance;
};

#endif
