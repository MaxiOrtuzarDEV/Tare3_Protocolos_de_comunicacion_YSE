#include "PropioProtocolo.h"

#include "PropioProtocolo.h"
#include <cstring>

ByteVector construirProtocoloPropio(const PropioProtocolo &protocolo)
{
    ByteVector salida;

    // Byte 0: CMD (4 bits altos) + relleno (4 bits bajos = 0)
    BYTE byte0 = (protocolo.cmd & 0x0F) << 4;
    salida.push_back(byte0);

    // Byte 1: longitud_de_dato (6 bits bajos) + 2 bits de relleno
    BYTE byte1 = protocolo.longitud_de_dato & 0x3F;
    salida.push_back(byte1);

    // Datos (hasta 63 bytes)
    for (int i = 0; i < protocolo.longitud_de_dato && i < 63; ++i)
    {
        salida.push_back(protocolo.dato[i]);
    }

    // FCS
    salida.push_back(protocolo.fcs);

    return salida;
}

bool parsearProtocoloPropio(const ByteVector &entrada, PropioProtocolo &protocolo)
{
    if (entrada.size() < 3) // Mínimo: cmd + longitud + fcs
        return false;

    // Parsear CMD (4 bits altos del primer byte)
    protocolo.cmd = (entrada[0] >> 4) & 0x0F;

    // Parsear longitud (6 bits bajos del segundo byte)
    protocolo.longitud_de_dato = entrada[1] & 0x3F;

    // Verificar que tengamos suficientes bytes
    if (entrada.size() < (2 + protocolo.longitud_de_dato + 1))
        return false;

    // Limpiar datos
    memset(protocolo.dato, 0, sizeof(protocolo.dato));

    // Copiar datos
    for (int i = 0; i < protocolo.longitud_de_dato && i < 63; ++i)
    {
        protocolo.dato[i] = entrada[2 + i];
    }

    // FCS está al final
    protocolo.fcs = entrada[2 + protocolo.longitud_de_dato];

    return true;
}

BYTE calcularFCS(const PropioProtocolo &protocolo)
{
    // FCS simple: XOR de todos los bytes
    BYTE fcs = 0;

    // XOR con CMD
    fcs ^= (protocolo.cmd << 4);

    // XOR con longitud
    fcs ^= protocolo.longitud_de_dato;

    // XOR con datos
    for (int i = 0; i < protocolo.longitud_de_dato && i < 63; ++i)
    {
        fcs ^= protocolo.dato[i];
    }

    return fcs;
}