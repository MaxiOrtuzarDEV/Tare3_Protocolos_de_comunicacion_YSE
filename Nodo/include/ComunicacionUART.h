#ifndef COMUNICACION_UART_H
#define COMUNICACION_UART_H

#include "Tipos_de_Datos.h"

class ComunicacionUART
{
public:
    ComunicacionUART(const std::string &dispositivo, int baudios = 115200);
    ~ComunicacionUART();

    bool abrir();
    void cerrar();
    bool estaAbierto() const;

    int enviar(const ByteVector &mensaje);
    ByteVector recibir();

private:
    std::string dispositivo_;
    int baudios_;
    int descriptor_;
    bool abierto_;
};

#endif
