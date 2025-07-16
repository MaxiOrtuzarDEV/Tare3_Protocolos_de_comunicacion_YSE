#include "red.h"
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Configuración OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET     16
#define OLED_SDA        4
#define OLED_SCL       15

// Configuración LoRa
#define SF_LORA 7
#define BW_LORA 250000L
#define CRC_LORA 1
#define TX_POWER_LORA 15

// Configuración SLIP
#define SLIP_END     0xC0
#define SLIP_ESC     0xDB
#define SLIP_ESC_END 0xDC
#define SLIP_ESC_ESC 0xDD

// LED integrado
#define LED_PIN 25

// Tamaños de buffer
#define BUFFER_SIZE 512
#define MAX_PACKET_SIZE 255

// Estructura IPv4 simplificada
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

// Estructura protocolo propio
struct PropioProtocolo {
    uint8_t cmd;
    uint8_t longitud_de_dato;
    uint8_t dato[63];
    uint8_t fcs;
};

// Variables globales
Red red;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

uint8_t buffer_uart[BUFFER_SIZE];
uint8_t buffer_slip[BUFFER_SIZE];
uint8_t buffer_lora[MAX_PACKET_SIZE];
int uart_pos = 0;
bool led_state = false;
uint16_t mi_ip = 0x3; // IP por defecto, se puede cambiar según el kit

// Prototipos de funciones
void procesarMensajeUART();
void procesarMensajeLoRa();
bool decodificarSLIP(uint8_t* entrada, int len_entrada, uint8_t* salida, int* len_salida);
bool codificarSLIP(uint8_t* entrada, int len_entrada, uint8_t* salida, int* len_salida);
bool parsearIPv4(uint8_t* datos, int len, IPv4Packet* paquete);
void construirIPv4(IPv4Packet* paquete, uint8_t* buffer, int* len);
uint8_t calcularChecksum(IPv4Packet* paquete);
void procesarProtocoloPropio(PropioProtocolo* comando);
void enviarPorUART(IPv4Packet* paquete);
void enviarPorLoRa(IPv4Packet* paquete);
void mostrarEnOLED(String mensaje);
void mostrarImagenPrueba();
uint8_t calcularFCS(PropioProtocolo* comando);

void setup() {
    Serial.begin(115200);
    
    // Inicializar LED
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    
    // Inicializar I2C para OLED
    Wire.begin(OLED_SDA, OLED_SCL);
    
    // Inicializar OLED
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("Error al inicializar OLED");
    } else {
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0,0);
        display.println("Modem LoRa");
        display.println("Iniciando...");
        display.display();
    }
    
    // Inicializar LoRa
    red.begin(SF_LORA, BW_LORA, CRC_LORA, TX_POWER_LORA);
    
    // Configurar IP del nodo (se puede leer desde EEPROM o configurar)
    mi_ip = 0x3;
    
    Serial.println("Modem LoRa inicializado");
    Serial.print("IP del nodo: 0x");
    Serial.println(mi_ip, HEX);
    
    mostrarEnOLED("Modem LoRa\nIP: 0x" + String(mi_ip, HEX) + "\nListo");
}

void loop() {
    // Procesar mensajes no bloqueantes
    procesarMensajeUART();
    procesarMensajeLoRa();
    
    // Pequeña pausa para evitar saturar el procesador
    delay(1);
}

void procesarMensajeUART() {
    // Leer datos del puerto serial de forma no bloqueante
    while (Serial.available() > 0 && uart_pos < BUFFER_SIZE - 1) {
        uint8_t byte_recibido = Serial.read();
        buffer_uart[uart_pos++] = byte_recibido;
        
        // Si encontramos el final del frame SLIP
        if (byte_recibido == SLIP_END && uart_pos > 1) {
            // Decodificar SLIP
            int len_decodificado;
            if (decodificarSLIP(buffer_uart, uart_pos, buffer_slip, &len_decodificado)) {
                // Parsear IPv4
                IPv4Packet paquete;
                if (parsearIPv4(buffer_slip, len_decodificado, &paquete)) {
                    // Procesar según las reglas del protocolo
                    if (paquete.ip_destino == mi_ip ) {
                        
                        PropioProtocolo comando;
                        switch(paquete.protocolo)
                        {
                            case 5:
                                comando.cmd = 5;
                                procesarProtocoloPropio(&comando);
                                break;
                            
                            case 6:
                                comando.cmd = 6;
                                procesarProtocoloPropio(&comando);
                                break;
                            case 7:
                                comando.cmd = 7;
                                procesarProtocoloPropio(&comando);
                                break;
                        }
                        procesarProtocoloPropio(&comando);
                        
                    }
                    else if (paquete.ip_destino != mi_ip || paquete.ip_destino == 0xFFFF) {
                        // Reenviar por LoRa si no es protocolo propio
                        if (paquete.protocolo != 0) {
                            enviarPorLoRa(&paquete);
                        }
                    }
                }
            }
            uart_pos = 0; // Reiniciar buffer
        }
    }
    
    // Reiniciar buffer si se llena
    if (uart_pos >= BUFFER_SIZE - 1) {
        uart_pos = 0;
    }
}

void procesarMensajeLoRa() {
    // Verificar si hay datos disponibles en LoRa
    if (red.dataDisponible()) {
        int len_recibido = red.getData(buffer_lora, MAX_PACKET_SIZE);
        
        if (len_recibido > 0) {
            // Parsear como IPv4
            IPv4Packet paquete;
            if (parsearIPv4(buffer_lora, len_recibido, &paquete)) {
                // Solo reenviar por UART si es para este nodo o broadcast
                if (paquete.ip_destino == mi_ip || paquete.ip_destino == 0xFFFF) {
                    enviarPorUART(&paquete);
                }
            }
        }
    }
}

bool decodificarSLIP(uint8_t* entrada, int len_entrada, uint8_t* salida, int* len_salida) {
    int pos_salida = 0;
    bool escape_next = false;
    
    for (int i = 0; i < len_entrada; i++) {
        if (escape_next) {
            if (entrada[i] == SLIP_ESC_END) {
                salida[pos_salida++] = SLIP_END;
            } else if (entrada[i] == SLIP_ESC_ESC) {
                salida[pos_salida++] = SLIP_ESC;
            } else {
                return false; // Error en decodificación
            }
            escape_next = false;
        } else if (entrada[i] == SLIP_ESC) {
            escape_next = true;
        } else if (entrada[i] == SLIP_END) {
            // Fin del frame (ignorar al final)
            continue;
        } else {
            salida[pos_salida++] = entrada[i];
        }
    }
    
    *len_salida = pos_salida;
    return true;
}

bool codificarSLIP(uint8_t* entrada, int len_entrada, uint8_t* salida, int* len_salida) {
    int pos_salida = 0;
    
    // Agregar SLIP_END al inicio
    salida[pos_salida++] = SLIP_END;
    
    for (int i = 0; i < len_entrada; i++) {
        if (entrada[i] == SLIP_END) {
            salida[pos_salida++] = SLIP_ESC;
            salida[pos_salida++] = SLIP_ESC_END;
        } else if (entrada[i] == SLIP_ESC) {
            salida[pos_salida++] = SLIP_ESC;
            salida[pos_salida++] = SLIP_ESC_ESC;
        } else {
            salida[pos_salida++] = entrada[i];
        }
    }
    
    // Agregar SLIP_END al final
    salida[pos_salida++] = SLIP_END;
    
    *len_salida = pos_salida;
    return true;
}

bool parsearIPv4(uint8_t* datos, int len, IPv4Packet* paquete) {
    if (len < 8) return false; // Mínimo para cabecera IPv4
    
    // Parsear campos de la cabecera
    uint16_t campo1 = (datos[0] << 8) | datos[1];
    paquete->flag_fragmento = (campo1 >> 14) & 0x3;
    paquete->offset_fragmento = campo1 & 0x0FFF;
    
    paquete->longitud_total = datos[2];
    paquete->relleno = datos[3];
    paquete->identificador = (datos[4] << 8) | datos[5];
    paquete->protocolo = datos[6];
    paquete->checksum = datos[7];
    paquete->ip_origen = (datos[8] << 8) | datos[9];
    paquete->ip_destino = (datos[10] << 8) | datos[11];
    
    // Copiar datos
    int datos_len = len - 12;
    if (datos_len > 0) {
        paquete->datos_len = (datos_len > MAX_PACKET_SIZE) ? MAX_PACKET_SIZE : datos_len;
        memcpy(paquete->datos, &datos[12], paquete->datos_len);
    } else {
        paquete->datos_len = 0;
    }
    
    return true;
}

void construirIPv4(IPv4Packet* paquete, uint8_t* buffer, int* len) {
    // Construir cabecera IPv4
    uint16_t campo1 = (paquete->flag_fragmento << 14) | (paquete->offset_fragmento & 0x0FFF);
    buffer[0] = (campo1 >> 8) & 0xFF;
    buffer[1] = campo1 & 0xFF;
    
    buffer[2] = paquete->longitud_total;
    buffer[3] = paquete->relleno;
    buffer[4] = (paquete->identificador >> 8) & 0xFF;
    buffer[5] = paquete->identificador & 0xFF;
    buffer[6] = paquete->protocolo;
    buffer[7] = paquete->checksum;
    buffer[8] = (paquete->ip_origen >> 8) & 0xFF;
    buffer[9] = paquete->ip_origen & 0xFF;
    buffer[10] = (paquete->ip_destino >> 8) & 0xFF;
    buffer[11] = paquete->ip_destino & 0xFF;
    
    // Copiar datos
    memcpy(&buffer[12], paquete->datos, paquete->datos_len);
    
    *len = 12 + paquete->datos_len;
}

uint8_t calcularChecksum(IPv4Packet* paquete) {
    // Checksum simple de la cabecera (sin IPs)
    uint8_t checksum = 0;
    uint16_t campo1 = (paquete->flag_fragmento << 14) | (paquete->offset_fragmento & 0x0FFF);
    
    checksum ^= (campo1 >> 8) & 0xFF;
    checksum ^= campo1 & 0xFF;
    checksum ^= paquete->longitud_total;
    checksum ^= paquete->relleno;
    checksum ^= (paquete->identificador >> 8) & 0xFF;
    checksum ^= paquete->identificador & 0xFF;
    checksum ^= paquete->protocolo;
    
    return checksum;
}

void procesarProtocoloPropio(PropioProtocolo* comando) {
    switch (comando->cmd) {
        case 5: // Comando de prueba
            mostrarImagenPrueba();
            break;
            
        case 6: // Cambiar estado LED
            led_state = !led_state;
            digitalWrite(LED_PIN, led_state ? HIGH : LOW);
            Serial.print("LED cambiado a: ");
            Serial.println(led_state ? "ON" : "OFF");
            break;
            
        case 7: // Mostrar mensaje en OLED
            if (comando->longitud_de_dato > 0) {
                String mensaje = "";
                for (int i = 0; i < comando->longitud_de_dato; i++) {
                    mensaje += (char)comando->dato[i];
                }
                mostrarEnOLED(mensaje);
            }
            break;
            
        default:
            Serial.print("Comando desconocido: ");
            Serial.println(comando->cmd);
            break;
    }
}

void enviarPorUART(IPv4Packet* paquete) {
    uint8_t buffer_ipv4[BUFFER_SIZE];
    int len_ipv4;
    
    // Recalcular checksum
    paquete->checksum = calcularChecksum(paquete);
    
    // Construir paquete IPv4
    construirIPv4(paquete, buffer_ipv4, &len_ipv4);
    
    // Codificar en SLIP
    uint8_t buffer_slip_tx[BUFFER_SIZE];
    int len_slip;
    
    if (codificarSLIP(buffer_ipv4, len_ipv4, buffer_slip_tx, &len_slip)) {
        // Enviar por UART
        Serial.write(buffer_slip_tx, len_slip);
    }
}

void enviarPorLoRa(IPv4Packet* paquete) {
    uint8_t buffer_ipv4[MAX_PACKET_SIZE];
    int len_ipv4;
    
    // Recalcular checksum
    paquete->checksum = calcularChecksum(paquete);
    
    // Construir paquete IPv4
    construirIPv4(paquete, buffer_ipv4, &len_ipv4);
    
    // Enviar por LoRa
    red.transmite_data(buffer_ipv4, len_ipv4);
}

void mostrarEnOLED(String mensaje) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    // Dividir mensaje en líneas
    int inicio = 0;
    int linea = 0;
    int max_lineas = 8;
    int max_chars_por_linea = 21;
    
    while (inicio < mensaje.length() && linea < max_lineas) {
        String linea_texto = mensaje.substring(inicio, inicio + max_chars_por_linea);
        display.println(linea_texto);
        inicio += max_chars_por_linea;
        linea++;
    }
    
    display.display();
}

void mostrarImagenPrueba() {
    display.clearDisplay();
    
    // Crear un patrón de prueba simple
    for (int i = 0; i < SCREEN_WIDTH; i += 8) {
        for (int j = 0; j < SCREEN_HEIGHT; j += 8) {
            display.drawRect(i, j, 8, 8, SSD1306_WHITE);
        }
    }
    
    // Agregar texto
    display.setTextSize(1);
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    display.setCursor(20, 28);
    display.println("PRUEBA OK");
    
    display.display();
    
    Serial.println("Imagen de prueba mostrada en OLED");
}

uint8_t calcularFCS(PropioProtocolo* comando) {
    uint8_t fcs = 0;
    fcs ^= comando->cmd;
    fcs ^= comando->longitud_de_dato;
    
    for (int i = 0; i < comando->longitud_de_dato; i++) {
        fcs ^= comando->dato[i];
    }
    
    return fcs;
}

