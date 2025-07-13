#include "include/IPv4.h"

bool parsearIPv4(const ByteVector &entrada, IPv4 &salida)
{
    if (entrada.size() < 11)
        return false;

    salida.flag_fragmento = (entrada[0] >> 4) & 0x0F;
    salida.offset_fragmento = ((entrada[0] & 0x0F) << 8) | entrada[1];
    salida.longitud_total = entrada[2];
    salida.identificador = (entrada[3] << 8) | entrada[4];
    salida.protocolo = entrada[5];
    salida.checksum = entrada[6];
    salida.ip_origen = (entrada[7] << 8) | entrada[8];
    salida.ip_destino = (entrada[9] << 8) | entrada[10];

    // Los datos empiezan desde la posición 11
    salida.datos.clear();
    for (size_t i = 11; i < entrada.size(); ++i)
    {
        salida.datos.push_back(entrada[i]);
    }

    return true;
}

ByteVector construirIPv4(const IPv4 &entrada)
{
    ByteVector salida;

    // Byte 0: Flag fragmento (4 bits altos) + Offset fragmento (4 bits altos)
    BYTE byte0 = ((entrada.flag_fragmento & 0x0F) << 4) | ((entrada.offset_fragmento >> 8) & 0x0F);
    // Byte 1: Offset fragmento (8 bits bajos)
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

    // Agregar los datos
    for (ByteVector::const_iterator it = entrada.datos.begin(); it != entrada.datos.end(); ++it)
    {
        salida.push_back(*it);
    }

    return salida;
}

BYTE calcularChecksum(const IPv4 &paquete)
{
    // Checksum simple de la cabecera (sin origen y destino)
    uint16_t suma = 0;

    suma += (paquete.flag_fragmento << 4) | (paquete.offset_fragmento >> 8);
    suma += paquete.offset_fragmento & 0xFF;
    suma += paquete.longitud_total;
    suma += paquete.identificador >> 8;
    suma += paquete.identificador & 0xFF;
    suma += paquete.protocolo;

    // Complemento a 1 del resultado
    suma = (suma & 0xFF) + (suma >> 8);
    return (~suma) & 0xFF;
}