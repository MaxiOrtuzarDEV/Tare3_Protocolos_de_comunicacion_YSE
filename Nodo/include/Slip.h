#ifndef SLIP_H
#define SLIP_H

#include "Tipos_de_Datos.h"

// Constantes SLIP
#define SLIP_END 0xC0
#define SLIP_ESC 0xDB
#define SLIP_ESC_END 0xDC
#define SLIP_ESC_ESC 0xDD

bool SLIP_encode(const ByteVector &entrada, ByteVector &salida);
bool SLIP_decode(const ByteVector &entrada, ByteVector &salida);

#endif // SLIP_H