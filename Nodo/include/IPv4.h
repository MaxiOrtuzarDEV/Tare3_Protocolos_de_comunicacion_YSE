#ifndef IPV4_H
#define IPV4_H

#include "tipos_de_datos.h"
#include <cstdint>

struct IPv4
{
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

bool parsearIPv4(const ByteVector &entrada, IPv4 &salida);
ByteVector construirIPv4(const IPv4 &entrada);
BYTE calcularChecksum(const IPv4 &paquete);

#endif // IPV4_H
