#ifndef PROPIO_PROTOCOLO_H
#define PROPIO_PROTOCOLO_H

#include "Tipos_de_Datos.h"

// Estructura del Protocolo Propio según especificaciones:
// 4 bits CMD + 4 bits relleno + 6 bits largo de data + 63 bytes de data + FCS
struct PropioProtocolo
{
    BYTE cmd;              // 4 bits CMD (en los 4 bits altos del byte)
    BYTE longitud_de_dato; // 6 bits para longitud (0-63)
    BYTE dato[63];         // Máximo 63 bytes de datos
    BYTE fcs;
};

// Funciones para manejar el protocolo
ByteVector construirProtocoloPropio(const PropioProtocolo &protocolo);
bool parsearProtocoloPropio(const ByteVector &entrada, PropioProtocolo &protocolo);
BYTE calcularFCS(const PropioProtocolo &protocolo);

#endif // PROPIO_PROTOCOLO_H