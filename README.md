# Sistema de Comunicación LoRa

Un sistema completo de comunicación inalámbrica usando tecnología LoRa con protocolo IPv4 simplificado y protocolo SLIP para la transmisión de datos.

## 🚀 Características

- **Comunicación LoRa**: Transmisión de largo alcance y bajo consumo
- **Protocolo IPv4 simplificado**: Implementación básica para ruteo de paquetes
- **Protocolo SLIP**: Codificación/decodificación para transmisión serial
- **Múltiples tipos de mensajes**: Unicast, Broadcast, Hello, ACK, comandos internos
- **Interfaz OLED**: Visualización de mensajes en pantalla
- **Control de LED**: Comandos remotos para control de hardware
- **Sistema de ACK**: Confirmación de recepción con reintentos automáticos
- **Descubrimiento de nodos**: Protocolo Hello para detectar nodos disponibles

## 📋 Requisitos

### Hardware
- **ESP32** o microcontrolador compatible
- **Módulo LoRa** (compatible con la librería `red.h`)
- **Pantalla OLED** (128x64, controlador SSD1306)
- **LED integrado**
- **Conexión UART** al PC

### Software
- **Arduino IDE** para el firmware del modem
- **Compilador C++** (g++) para la aplicación del nodo
- **Sistema Linux** (para comunicación UART)

### Librerías Arduino
```cpp
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "red.h"  // Librería específica del módulo LoRa
```

## 🔧 Instalación

### 1. Configuración del Modem (Arduino)

1. Conectar el hardware según el esquema de pines:
   ```cpp
   #define OLED_SDA        4
   #define OLED_SCL       15
   #define OLED_RESET     16
   #define LED_PIN        25
   ```

2. Cargar el firmware `Modem.ino` al ESP32

3. Configurar la IP del nodo (por defecto: 0x3):
   ```cpp
   mi_ip = 0x3;  // Cambiar según sea necesario
   ```

### 2. Compilación de la Aplicación del Nodo

```bash
# Clonar o descargar el proyecto
cd sistema-lora

# Compilar usando Make
make

# O compilar manualmente
g++ -Wall -Wextra -std=c++0x -Iinclude src/*.cpp -o bin/app
```

## 🚀 Uso

### Ejecutar el Nodo

```bash
# Ejecutar con IP por defecto (0x0003)
make run

# O ejecutar con IP personalizada
./bin/app 0x0010
```

### Menú Principal

```
=============== MENÚ PRINCIPAL ===============
IP del nodo: 0x3
1. Ver nodos disponibles
2. Enviar mensaje Hello
3. Comandos internos del modem
4. Enviar mensajes a otros nodos
5. Salir
```

### Comandos Disponibles

#### 1. Descubrimiento de Nodos
- **Hello**: Envía mensaje broadcast para anunciar presencia
- **Ver nodos**: Muestra tabla de nodos detectados con timestamps

#### 2. Mensajería
- **Unicast**: Mensaje directo a un nodo específico (requiere ACK)
- **Broadcast**: Mensaje a todos los nodos (sin ACK)

#### 3. Comandos Internos
- **Prueba**: Muestra patrón de prueba en OLED
- **LED**: Cambia estado del LED integrado
- **OLED**: Envía mensaje para mostrar en pantalla

## 📡 Protocolos Implementados

### Protocolo IPv4 Simplificado
```cpp
struct IPv4Packet {
    uint8_t flag_fragmento : 2;
    uint16_t offset_fragmento : 12;
    uint8_t longitud_total;
    uint8_t relleno;
    uint16_t identificador;
    uint8_t protocolo;
    uint8_t checksum;
    uint16_t ip_origen;
    uint16_t ip_destino;
    uint8_t datos[MAX_PACKET_SIZE];
    uint8_t datos_len;
};
```

### Tipos de Protocolo
- **0**: Protocolo propio (comandos internos)
- **1**: ACK (confirmación)
- **2**: Mensaje Unicast
- **3**: Mensaje Broadcast
- **4**: Hello (descubrimiento)
- **5**: Comando de prueba
- **6**: Control de LED
- **7**: Mensaje OLED

### Protocolo SLIP
- **SLIP_END**: 0xC0 (marcador de fin)
- **SLIP_ESC**: 0xDB (carácter de escape)
- **SLIP_ESC_END**: 0xDC (END escapado)
- **SLIP_ESC_ESC**: 0xDD (ESC escapado)

## 🔧 Configuración

### Parámetros LoRa
```cpp
#define SF_LORA 7           // Spreading Factor
#define BW_LORA 250000L     // Bandwidth
#define CRC_LORA 1          // CRC habilitado
#define TX_POWER_LORA 15    // Potencia de transmisión
```

### Configuración UART
```cpp
Serial.begin(115200);       // Velocidad de baudios
ComunicacionUART uart("/dev/ttyUSB0", 115200);
```

## 🛠️ Estructura del Proyecto

```
sistema-lora/
├── Modem.ino              # Firmware del modem LoRa
├── Makefile               # Archivo de compilación
├── src/
│   ├── main.cpp           # Punto de entrada
│   ├── Nodo.cpp           # Lógica principal del nodo
│   └── [otros archivos]   # Implementaciones adicionales
├── include/
│   ├── Nodo.h             # Definiciones del nodo
│   ├── ComunicacionUART.h # Interfaz UART
│   └── [otros headers]    # Cabeceras adicionales
├── bin/                   # Ejecutables compilados
└── obj/                   # Archivos objeto
```

## 🐛 Resolución de Problemas

### Error de Conexión UART
```
Error: No se pudo abrir el puerto UART
```
**Solución**: Verificar que el dispositivo esté conectado en `/dev/ttyUSB0` y que el usuario tenga permisos:
```bash
sudo chmod 666 /dev/ttyUSB0
# O agregar usuario al grupo dialout
sudo usermod -a -G dialout $USER
```

### Error de Inicialización OLED
```
Error al inicializar OLED
```
**Solución**: Verificar conexiones I2C y dirección del dispositivo (0x3C)

### Nodo No Responde
1. Verificar que ambos nodos estén en la misma configuración LoRa
2. Comprobar que las IPs sean diferentes
3. Asegurar que el nodo destino esté activo y haya enviado Hello

## 📊 Monitoreo

### Mensajes del Sistema
- `[+]`: Mensaje recibido exitosamente
- `[!]`: Error o advertencia
- `[✓]`: Operación completada
- `[...]`: Operación en progreso

### Logs de Ejemplo
```
[+] Mensaje unicast de nodo 0x10: Hola mundo
[✓] ACK enviado a nodo 0x10
[!] No se recibió ACK para ID 1234 después de 2 intentos
```

## 🤝 Contribuciones

1. Fork el proyecto
2. Crear una rama para la funcionalidad (`git checkout -b feature/nueva-funcionalidad`)
3. Commit los cambios (`git commit -am 'Agregar nueva funcionalidad'`)
4. Push a la rama (`git push origin feature/nueva-funcionalidad`)
5. Crear un Pull Request

## 📝 Licencia

Este proyecto está bajo la Licencia MIT. Ver el archivo `LICENSE` para más detalles.

## 📧 Contacto

Para preguntas o sugerencias, crear un issue en el repositorio del proyecto.

---

**Nota**: Este proyecto está diseñado para fines educativos y de desarrollo. Para uso en producción, se recomienda implementar medidas adicionales de seguridad y manejo de errores.
