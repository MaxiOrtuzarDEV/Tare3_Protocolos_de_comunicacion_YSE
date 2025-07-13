#include "include/IPv4.h"

bool parsearIPv4(const ByteVector &entrada, IPv4Paquete &salida)
{
    if (entrada.size() < 12)
        return false;

    salida.flag_fragmento = (entrada[0] >> 4) & 0x0F;
    salida.offset_fragmento = ((entrada[0] & 0x0F) << 8) | entrada[1];
    salida.longitud_total = entrada[2];
    salida.identificador = (entrada[3] << 8) | entrada[4];
    salida.protocolo = entrada[5];
    salida.checksum = entrada[6];
    salida.ip_origen = (entrada[7] << 8) | entrada[8];
    salida.ip_destino = (entrada[9] << 8) | entrada[10];

    salida.datos.assign(entrada.begin() + 11, entrada.end());

    return true;
}

ByteVector construirIPv4(const IPv4Paquete &entrada)
{
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

    salida.insert(salida.end(), entrada.datos.begin(), entrada.datos.end());

    return salida;
}

BYTE calcularChecksum(const IPv4Paquete &paquete)
{
    // Puedes hacer un checksum simple para la cabecera si el protocolo lo pide
    return 0; // Por ahora, no implementado
}
