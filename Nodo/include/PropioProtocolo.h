#ifndef PROPIO_PROTOCOLO_H
#define PROPIO_PROTOCOLO_H

#include "include/Tipos_de_Datos.h"

struct PropioProtocolo
{
    BYTE cmd;
    BYTE longitud_de_dato;
    BYTE dato[63];
};

#endif