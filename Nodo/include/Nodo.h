#ifndef NODO_H
#define NODO_H

#include "include/Tipos_de_Datos.h"

class Nodo
{
private:
    ComunicacionUART uart;

    uint16_t ip_nodo;

    void menu();
    void actualizarMensajesEntrantes();
    void verNodos();
    void enviarComandosAlModem();
    void enviarHello();
    void enviarMensajesAOtrosNodos();
    void enviarComandosANodos();

public:
    Nodo(uint16_t ip);
    ~Nodo();

    void run();
};

#endif