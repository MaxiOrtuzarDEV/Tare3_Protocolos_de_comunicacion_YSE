#include <heltec.h>
#include <SPI.h>
#include <LoRa.h>
#include <vector>

// Definiciones de tipos (similar a Tipos_de_Datos.h)
typedef uint8_t BYTE;
typedef std::vector<BYTE> ByteVector;

// Constantes SLIP (similar a Slip.h)
#define SLIP_END 0xC0
#define SLIP_ESC 0xDB
#define SLIP_ESC_END 0xDC
#define SLIP_ESC_ESC 0xDD

// Estructura IPv4 simplificado (similar a IPv4.h)
struct IPv4 {
    BYTE flag_fragmento;
    uint16_t offset_fragmento;
    BYTE longitud_total;
    uint16_t identificador;
    BYTE protocolo;
    BYTE checksum;
    uint16_t ip_origen;
    uint16_t ip_destino;
    ByteVector datos;
};

// Estructura Protocolo Propio (similar a PropioProtocolo.h)
struct PropioProtocolo {
    BYTE cmd;
    BYTE longitud_de_dato;
    BYTE dato[63];
    BYTE fcs;
};

// Configuración LoRa
#define BAND 915E6  // Frecuencia LoRa - ajustar según región
#define SYNC_WORD 0x12  // Palabra de sincronización para la red

// IP del modem (debe coincidir con la configurada en el Nodo)
uint16_t ip_modem = 0x0001; // Cambiar según el kit asignado

// Buffer para UART
String uartBuffer = "";
bool slipPacketComplete = false;

void setup() {
    // Inicializar pantalla OLED
    Heltec.begin(true /*DisplayEnable Enable*/, false /*LoRa Disable*/, true /*Serial Enable*/);
    
    // Configurar UART
    Serial.begin(115200);
    
    // Inicializar LoRa
    LoRa.setPins(18, 14, 26); // CS, RST, IRQ
    if (!LoRa.begin(BAND)) {
        Serial.println("Error al iniciar LoRa!");
        while (1);
    }
    
    // Configurar parámetros LoRa
    LoRa.setSyncWord(SYNC_WORD);
    LoRa.setSpreadingFactor(7);
    LoRa.setSignalBandwidth(125E3);
    
    // Mostrar mensaje inicial
    Heltec.display->clear();
    Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
    Heltec.display->setFont(ArialMT_Plain_10);
    Heltec.display->drawString(0, 0, "Modem LoRa V1");
    Heltec.display->drawString(0, 12, "IP: 0x" + String(ip_modem, HEX));
    Heltec.display->drawString(0, 24, "Esperando datos...");
    Heltec.display->display();
}

void loop() {
    // Procesar datos recibidos por UART (desde Nodo)
    procesarUART();
    
    // Procesar paquetes recibidos por LoRa
    procesarLoRa();
}

void procesarUART() {
    while (Serial.available()) {
        char c = Serial.read();
        
        // Detectar inicio/fin de paquete SLIP
        if (c == SLIP_END) {
            if (slipPacketComplete) {
                // Paquete completo, procesarlo
                ByteVector slipData;
                for (size_t i = 0; i < uartBuffer.length(); i++) {
                    slipData.push_back(uartBuffer[i]);
                }
                
                ByteVector decoded;
                if (SLIP_decode(slipData, decoded)) {
                    IPv4 paquete;
                    if (parsearIPv4(decoded, paquete)) {
                        manejarPaqueteDesdeNodo(paquete);
                    }
                }
                
                uartBuffer = "";
                slipPacketComplete = false;
            } else {
                slipPacketComplete = true;
            }
        } else if (slipPacketComplete) {
            uartBuffer += c;
        }
    }
}

void procesarLoRa() {
    int packetSize = LoRa.parsePacket();
    if (packetSize) {
        ByteVector receivedData;
        
        while (LoRa.available()) {
            receivedData.push_back(LoRa.read());
        }
        
        IPv4 paquete;
        if (parsearIPv4(receivedData, paquete)) {
            manejarPaqueteDesdeLoRa(paquete);
        }
    }
}

void manejarPaqueteDesdeNodo(const IPv4 &paquete) {
    // Verificar checksum
    if (calcularChecksum(paquete) != paquete.checksum) {
        Serial.println("Checksum incorrecto, descartando paquete");
        return;
    }
    
    // Verificar IP de origen
    if (paquete.ip_origen != ip_modem) {
        Serial.println("IP de origen no coincide, descartando paquete");
        return;
    }
    
    // Procesar según protocolo y destino
    if (paquete.ip_destino == ip_modem && paquete.protocolo == 0) {
        // Protocolo propio para comando interno
        PropioProtocolo proto;
        if (parsearProtocoloPropio(paquete.datos, proto)) {
            ejecutarComandoPropio(proto);
        }
    } else if (paquete.ip_destino != ip_modem || paquete.ip_destino == 0xFFFF) {
        // Enviar por LoRa (excepto protocolo propio para este nodo)
        if (paquete.protocolo != 0) {
            ByteVector ipv4Data = construirIPv4(paquete);
            LoRa.beginPacket();
            for (size_t i = 0; i < ipv4Data.size(); i++) {
                LoRa.write(ipv4Data[i]);
            }
            LoRa.endPacket();
            
            // Mostrar en pantalla
            mostrarEnPantalla("Enviado por LoRa", "Dest: 0x" + String(paquete.ip_destino, HEX));
        }
    }
}

void manejarPaqueteDesdeLoRa(const IPv4 &paquete) {
    // Verificar checksum
    if (calcularChecksum(paquete) != paquete.checksum) {
        Serial.println("Checksum incorrecto, descartando paquete LoRa");
        return;
    }
    
    // Solo procesar si es para este nodo o broadcast
    if (paquete.ip_destino == ip_modem || paquete.ip_destino == 0xFFFF) {
        // Enviar por UART al Nodo
        ByteVector ipv4Data = construirIPv4(paquete);
        ByteVector slipData;
        SLIP_encode(ipv4Data, slipData);
        
        Serial.write(SLIP_END);
        for (size_t i = 0; i < slipData.size(); i++) {
            Serial.write(slipData[i]);
        }
        Serial.write(SLIP_END);
        
        // Mostrar en pantalla según protocolo
        switch (paquete.protocolo) {
            case 1: // ACK
                mostrarEnPantalla("ACK recibido", "De: 0x" + String(paquete.ip_origen, HEX));
                break;
            case 2: // Unicast
                mostrarEnPantalla("Unicast recibido", "De: 0x" + String(paquete.ip_origen, HEX));
                break;
            case 3: // Broadcast
                mostrarEnPantalla("Broadcast recibido", "De: 0x" + String(paquete.ip_origen, HEX));
                break;
            case 5: // Prueba
                mostrarEnPantalla("Comando prueba", "Ejecutando...");
                break;
            case 6: // LED
                mostrarEnPantalla("Comando LED", "Cambiando estado");
                break;
            case 7: // OLED
                mostrarEnPantalla("Mensaje OLED", String((char*)paquete.datos.data()));
                break;
        }
    }
}

void ejecutarComandoPropio(const PropioProtocolo &proto) {
    // Verificar FCS
    if (calcularFCS(proto) != proto.fcs) {
        Serial.println("FCS incorrecto, descartando comando");
        return;
    }
    
    // Ejecutar comando según protocolo propio
    switch (proto.cmd) {
        case 5: // Comando de prueba
            mostrarImagenPrueba();
            break;
        case 6: // Cambiar estado LED
            toggleLED();
            break;
        case 7: // Mostrar en OLED
            mostrarMensajeOLED(proto);
            break;
        default:
            Serial.println("Comando propio desconocido");
            break;
    }
}

void mostrarImagenPrueba() {
    Heltec.display->clear();
    Heltec.display->drawXbm(0, 0, 128, 64, imagen_prueba_bits);
    Heltec.display->display();
}

void toggleLED() {
    static bool ledState = false;
    ledState = !ledState;
    digitalWrite(LED_BUILTIN, ledState);
}

void mostrarMensajeOLED(const PropioProtocolo &proto) {
    Heltec.display->clear();
    Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
    Heltec.display->setFont(ArialMT_Plain_10);
    
    String mensaje;
    for (int i = 0; i < proto.longitud_de_dato; i++) {
        mensaje += (char)proto.dato[i];
    }
    
    Heltec.display->drawStringMaxWidth(0, 0, 128, mensaje);
    Heltec.display->display();
}

void mostrarEnPantalla(String titulo, String mensaje) {
    Heltec.display->clear();
    Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
    Heltec.display->setFont(ArialMT_Plain_10);
    Heltec.display->drawString(0, 0, titulo);
    Heltec.display->drawString(0, 12, mensaje);
    Heltec.display->display();
}

// Funciones de parseo y construcción (similares a las del Nodo)
bool parsearIPv4(const ByteVector &entrada, IPv4 &salida) {
    if (entrada.size() < 11) return false;
    
    salida.flag_fragmento = (entrada[0] >> 4) & 0x0F;
    salida.offset_fragmento = ((entrada[0] & 0x0F) << 8) | entrada[1];
    salida.longitud_total = entrada[2];
    salida.identificador = (entrada[3] << 8) | entrada[4];
    salida.protocolo = entrada[5];
    salida.checksum = entrada[6];
    salida.ip_origen = (entrada[7] << 8) | entrada[8];
    salida.ip_destino = (entrada[9] << 8) | entrada[10];
    
    salida.datos.clear();
    for (size_t i = 11; i < entrada.size(); ++i) {
        salida.datos.push_back(entrada[i]);
    }
    
    return true;
}

ByteVector construirIPv4(const IPv4 &entrada) {
    ByteVector salida;
    
    BYTE byte0 = ((entrada.flag_fragmento & 0x0F) << 4) | ((entrada.offset_fragmento >> 8) & 0x0F);
    BYTE byte1 = entrada.offset_fragmento & 0xFF;
    
    salida.push_back(byte0);
    salida.push_back(byte1);
    salida.push_back(entrada.longitud_total);
    salida.push_back((entrada.identificador >> 8) & 0xFF);
    salida.push_back(entrada.identificador & 0xFF);
    salida.push_back(entrada.protocolo);
    salida.push_back(entrada.checksum);
    salida.push_back((entrada.ip_origen >> 8) & 0xFF);
    salida.push_back(entrada.ip_origen & 0xFF);
    salida.push_back((entrada.ip_destino >> 8) & 0xFF);
    salida.push_back(entrada.ip_destino & 0xFF);
    
    for (size_t i = 0; i < entrada.datos.size(); ++i) {
        salida.push_back(entrada.datos[i]);
    }
    
    return salida;
}

BYTE calcularChecksum(const IPv4 &paquete) {
    uint16_t suma = 0;
    
    suma += (paquete.flag_fragmento << 4) | (paquete.offset_fragmento >> 8);
    suma += paquete.offset_fragmento & 0xFF;
    suma += paquete.longitud_total;
    suma += paquete.identificador >> 8;
    suma += paquete.identificador & 0xFF;
    suma += paquete.protocolo;
    
    suma = (suma & 0xFF) + (suma >> 8);
    return (~suma) & 0xFF;
}

bool parsearProtocoloPropio(const ByteVector &entrada, PropioProtocolo &protocolo) {
    if (entrada.size() < 3) return false;
    
    protocolo.cmd = (entrada[0] >> 4) & 0x0F;
    protocolo.longitud_de_dato = entrada[1] & 0x3F;
    
    if (entrada.size() < (2 + protocolo.longitud_de_dato + 1)) return false;
    
    memset(protocolo.dato, 0, sizeof(protocolo.dato));
    for (int i = 0; i < protocolo.longitud_de_dato && i < 63; ++i) {
        protocolo.dato[i] = entrada[2 + i];
    }
    
    protocolo.fcs = entrada[2 + protocolo.longitud_de_dato];
    return true;
}

BYTE calcularFCS(const PropioProtocolo &protocolo) {
    BYTE fcs = 0;
    fcs ^= (protocolo.cmd << 4);
    fcs ^= protocolo.longitud_de_dato;
    
    for (int i = 0; i < protocolo.longitud_de_dato && i < 63; ++i) {
        fcs ^= protocolo.dato[i];
    }
    
    return fcs;
}

bool SLIP_encode(const ByteVector &entrada, ByteVector &salida) {
    salida.clear();
    salida.push_back(SLIP_END);
    
    for (size_t i = 0; i < entrada.size(); ++i) {
        BYTE b = entrada[i];
        if (b == SLIP_END) {
            salida.push_back(SLIP_ESC);
            salida.push_back(SLIP_ESC_END);
        } else if (b == SLIP_ESC) {
            salida.push_back(SLIP_ESC);
            salida.push_back(SLIP_ESC_ESC);
        } else {
            salida.push_back(b);
        }
    }
    
    salida.push_back(SLIP_END);
    return true;
}

bool SLIP_decode(const ByteVector &entrada, ByteVector &salida) {
    salida.clear();
    bool dentro = false;
    
    for (size_t i = 0; i < entrada.size(); ++i) {
        BYTE b = entrada[i];
        
        if (b == SLIP_END) {
            if (dentro && !salida.empty()) return true;
            dentro = true;
            continue;
        }
        
        if (!dentro) continue;
        
        if (b == SLIP_ESC) {
            if (i + 1 >= entrada.size()) return false;
            BYTE siguiente = entrada[++i];
            if (siguiente == SLIP_ESC_END) {
                salida.push_back(SLIP_END);
            } else if (siguiente == SLIP_ESC_ESC) {
                salida.push_back(SLIP_ESC);
            } else {
                return false;
            }
        } else {
            salida.push_back(b);
        }
    }
    
    return !salida.empty();
}

