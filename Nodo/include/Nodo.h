#ifndef NODO_H
#define NODO_H

#include "Tipos_de_Datos.h"
#include "ComunicacionUART.h"
#include "IPv4.h"
#include "PropioProtocolo.h"
#include <map>
#include <iostream>

struct ACKPendiente
{
    uint16_t ip_destino;
    uint16_t id_mensaje;
    int intentos;
    time_t tiempo_envio;
};

class Nodo
{
private:
    ComunicacionUART uart;
    uint16_t ip_nodo;
    uint16_t contador_id;
    std::map<uint16_t, time_t> tablaNodosHello;
    std::map<uint16_t, ACKPendiente> acksEsperando;

    // Métodos del menú
    void menu();
    void menuComandosInternos();
    void menuEnvioMensajes();

    // Métodos de comunicación
    void actualizarMensajesEntrantes();
    void enviarPaquete(const IPv4 &paquete);
    void enviarACK(uint16_t ip_destino, uint16_t id_mensaje);
    void enviarComandoAlModem(const PropioProtocolo &comando);
    bool esperarACK(uint16_t ip_destino, uint16_t id_mensaje, int max_intentos = 2);

    // Métodos de procesamiento de mensajes
    void procesarACK(const IPv4 &paquete);
    void procesarMensajeUnicast(const IPv4 &paquete);
    void procesarMensajeBroadcast(const IPv4 &paquete);
    void procesarHello(const IPv4 &paquete);
    void procesarComandoPrueba(const IPv4 &paquete);
    void procesarComandoLed(const IPv4 &paquete);
    void procesarComandoOLED(const IPv4 &paquete);

    // Métodos de envío
    void verNodos();
    void enviarHello();
    void enviarMensajeUnicast();
    void enviarMensajeBroadcast();
    void enviarComandoPrueba();
    void enviarComandoLed();
    void enviarMensajeOLED();

    // Utilidades
    uint16_t obtenerNuevoID();
    void limpiarPantalla();

public:
    Nodo(uint16_t ip);
    ~Nodo();
    void run();
};

#endif // NODO_H