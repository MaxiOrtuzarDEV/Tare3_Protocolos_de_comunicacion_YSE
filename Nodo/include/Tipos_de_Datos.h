#ifndef TIPOS_DE_DATOS_H
#define TIPOS_DE_DATOS_H

/*
    En este archivo se definen los tipos de datos utilizados
    en el proyecto. El objetivo es no confundir los tipos de datos
    y para que la lectura del codigo en desarrollo y en revision
    sea mas facil.
    xD

*/

#include <cstdint>
#include <vector>
#include <string>

typedef uint8_t BYTE;
typedef std::vector<BYTE> ByteVector;
typedef std::vector<std::string> StringVector;
typedef std::vector<int> IntVector;
typedef time_t Tiempo;

#endif // TIPOS_DE_DATOS_H