# Contribuir al ESP32 Modular Firmware

¡Gracias por tu interés en contribuir! Este documento te guiará en el proceso.

## 📋 Código de Conducta

- Sé respetuoso y constructivo
- Acepta feedback de manera positiva
- Ayuda a otros miembros de la comunidad

## 🐛 Reportar Bugs

Antes de reportar un bug:
1. Busca en los issues existentes
2. Usa la última versión del firmware
3. Verifica que no sea un problema de configuración

**Formato del Issue:**
```markdown
**Descripción del bug**
Descripción clara del problema

**Para Reproducir**
1. Paso 1
2. Paso 2
3. ...

**Comportamiento Esperado**
Lo que debería suceder

**Logs**
```
Pega aquí los logs del serial
```

**Hardware**
- ESP32 Model: 
- Sensores conectados:
- Versión del firmware:

**Configuración**
```json
{tu config.json}
```
```

## 💡 Sugerir Features

Usa el template de Feature Request:
- Describe el problema que soluciona
- Propón una solución
- Considera alternativas
- Agrega contexto adicional

## 🔧 Pull Requests

### Proceso

1. **Fork** el repositorio
2. **Crea una rama** desde `main`:
   ```bash
   git checkout -b feature/nombre-descriptivo
   ```
3. **Desarrolla** tu feature/fix
4. **Prueba** exhaustivamente
5. **Commit** con mensajes descriptivos
6. **Push** a tu fork
7. **Abre un PR** con descripción detallada

### Estándares de Código

#### C++ Style Guide

```cpp
// Nombres de clases: PascalCase
class MiClaseSensor : public BaseSensor {
    
    // Métodos públicos: camelCase
    bool begin() override;
    float readValue();
    
    // Métodos privados: camelCase
    void initializeHardware();
    
    // Variables miembro: camelCase con prefijo
    uint8_t sensorPin;
    bool isInitialized;
    
    // Constantes: UPPER_SNAKE_CASE
    static const uint8_t MAX_RETRIES = 3;
};

// Funciones globales: camelCase
void setupSensors();

// Variables globales: camelCase (evitar si es posible)
ConfigManager configManager;
```

#### Comentarios

```cpp
// Comentarios de una línea para código simple
int pin = 23;

/**
 * Comentarios de bloque para funciones/clases
 * 
 * @param id Identificador único del sensor
 * @param pin Pin GPIO a usar
 * @return true si inicializa correctamente
 */
bool initializeSensor(const String& id, uint8_t pin);

// TODO: Agregar soporte para sensor X
// FIXME: Bug al leer múltiples valores
```

#### Formato

- Indentación: 4 espacios (no tabs)
- Llaves en nueva línea para funciones/clases
- Llaves en misma línea para if/for/while
- Espacios alrededor de operadores
- Máximo 100 caracteres por línea

```cpp
// ✅ Correcto
if (sensor.isEnabled()) {
    float value = sensor.read();
    publishData(value);
}

// ❌ Incorrecto
if(sensor.isEnabled()){float value=sensor.read();publishData(value);}
```

### Checklist del PR

Antes de enviar tu PR, verifica:

- [ ] El código compila sin errores
- [ ] Probado en hardware real (si aplica)
- [ ] Documentación actualizada
- [ ] Ejemplos agregados (si es feature nueva)
- [ ] Commits con mensajes descriptivos
- [ ] Sin código comentado o debug prints
- [ ] Sin conflictos con `main`

### Tipos de Contribución

#### 🆕 Nuevo Sensor

1. Crea clase en `src/sensors/`
2. Hereda de `BaseSensor` o subclase apropiada
3. Implementa métodos requeridos
4. Integra en `SensorCore::loadSensors()`
5. Agrega ejemplo en `examples/`
6. Documenta en `docs/ADDING_SENSORS.md`

#### 📡 Nuevo Protocolo

1. Crea clase en `src/communication/`
2. Hereda de `BaseComm`
3. Implementa métodos requeridos
4. Integra en `CommCore::loadCommunication()`
5. Actualiza `ConfigManager` para parsing
6. Agrega ejemplo en `examples/`
7. Documenta en `docs/ADDING_PROTOCOLS.md`

#### 🐛 Bug Fix

1. Referencia el issue en el commit/PR
2. Agrega test/ejemplo que reproduce el bug
3. Explica la solución implementada

#### 📚 Documentación

1. Usa Markdown correcto
2. Agrega ejemplos de código
3. Incluye imágenes si ayuda
4. Actualiza tabla de contenidos si es necesario

## 🧪 Testing

### Manual Testing

```bash
# Compilar
pio run

# Subir
pio run --target upload

# Monitor
pio device monitor -b 115200
```

### Test Checklist

- [ ] Compila sin warnings
- [ ] Inicia correctamente
- [ ] Configuración se carga
- [ ] Sensores se inicializan
- [ ] Datos se leen correctamente
- [ ] Comunicación funciona
- [ ] Reconexión automática funciona
- [ ] Interfaz web accesible
- [ ] Configuración se guarda

## 📝 Convenciones de Commit

Usa [Conventional Commits](https://www.conventionalcommits.org/):

```
tipo(scope): descripción corta

Descripción más detallada si es necesario
```

**Tipos:**
- `feat`: Nueva funcionalidad
- `fix`: Corrección de bug
- `docs`: Cambios en documentación
- `style`: Formato (sin cambio de código)
- `refactor`: Refactorización
- `test`: Agregar tests
- `chore`: Cambios de build/herramientas

**Ejemplos:**
```
feat(sensors): agregar soporte para DHT22

Implementa clase DHT22Sensor que hereda de BaseSensor.
Incluye lectura de temperatura y humedad.
```

```
fix(mqtt): corregir reconexión automática

La reconexión fallaba después de 3 intentos.
Ahora se reinicia el contador correctamente.

Closes #42
```

```
docs(config): actualizar ejemplos de I2C

Agregar información sobre direcciones I2C comunes
y cómo detectarlas.
```

## 🎨 Assets y Recursos

- Diagramas: USA [Mermaid](https://mermaid.js.org/)
- Esquemas: Formato PNG o SVG
- Screenshots: Comprimir antes de subir

## 📦 Versionado

Seguimos [Semantic Versioning](https://semver.org/):
- MAJOR: Cambios incompatibles
- MINOR: Nueva funcionalidad compatible
- PATCH: Bug fixes compatibles

## 🏆 Reconocimientos

Los contribuidores se agregan automáticamente al README.

## ❓ Preguntas

¿Tienes dudas?
- Abre un [Discussion](https://github.com/andresfirmware/esp32-modular-firmware/discussions)
- Pregunta en el issue relacionado
- Revisa la [documentación](docs/)

¡Gracias por contribuir! 🚀
