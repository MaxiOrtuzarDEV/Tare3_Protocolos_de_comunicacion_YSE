#ifndef SLIP_H
#define SLIP_H

#include "tipos_de_datos.h" // Para usar ByteVector

bool SLIP_encode(const ByteVector &entrada, ByteVector &salida);
bool SLIP_decode(const ByteVector &entrada, ByteVector &salida);

#endif