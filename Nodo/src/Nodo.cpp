#include "include/Nodo.h"
#include "include/ComunicacionUART.h"
#include "SLIP.h"
#include "include/IPv4.h"
#include <chrono>
#include <iostream>
#include <unordered_map>

std::unordered_map<uint16_t, Tiempo> tablaNodosHello;

extern ComunicacionUART comunicacionUART;
extern std::unordered_map<uint16_t, Tiempo> tablaNodosHello;

Nodo::Nodo(uint16_t ip) : uart("/dev/ttyUSB0", 115200), ip_nodo(ip)
{
    uart.abrir();
}

Nodo::~Nodo()
{
}

void Nodo::actualizarMensajesEntrantes()
{
    std::string crudo = uart.recibir();

    if (crudo.empty())
        return;

    ByteVector entrada(crudo.begin(), crudo.end());
    ByteVector desempaquetado;

    if (!SLIP_decode(entrada, desempaquetado))
    {
        std::cerr << "[!] Error SLIP\n";
        return;
    }

    IPv4Paquete paquete;
    if (!parsearIPv4(desempaquetado, paquete))
        return;

    if (paquete.protocolo == 4)
    {
        auto ahora = std::chrono::steady_clock::now();
        tablaNodosHello[paquete.ip_origen] = ahora;

        std::cout << "[+] Hello recibido de nodo 0x"
                  << std::hex << paquete.ip_origen << std::dec << "\n";
    }
}

void Nodo::verNodos()
{
    std::cout << "Verificando nodos..." << std::endl;
    std::cout << "Mostrando lista de IPs..." << std::endl;

    auto ahora = std::chrono::steady_clock::now();

    if (tablaNodosHello.empty())
    {
        std::cout << "No se ha recibido ningún mensaje Hello aún.\n";
        return;
    }
    for (const auto &par : tablaNodosHello)
    {
        uint16_t ip = par.first;
        Tiempo recibido = par.second;

        auto segundos = std::chrono::duration_cast<std::chrono::seconds>(ahora - recibido).count();

        std::cout << "IP Nodo: 0x" << std::hex << ip << std::dec
                  << "  |  Recibido hace: " << segundos << " segundos\n";
    }
}

void Nodo::enviarHello()
{
    std::cout << "[+] Enviando mensaje Hello al módem..." << std::endl;

    IPv4Paquete paquete;

    paquete.flag_fragmento = 0;
    paquete.offset_fragmento = 0;
    paquete.longitud_total = 4;  // "hola"
    paquete.identificador = 1;   // puede ser cualquier número
    paquete.protocolo = 4;       // protocolo Hello
    paquete.checksum = 0;        // opcional
    paquete.ip_origen = 0x0010;  // ← asigna tu IP de nodo aquí
    paquete.ip_destino = 0xFFFF; // ← broadcast
    paquete.datos = {'h', 'o', 'l', 'a'};

    // Empaquetar
    ByteVector ipv4_bytes = construirIPv4(paquete);
    ByteVector slip_bytes;

    if (!SLIP_encode(ipv4_bytes, slip_bytes))
    {
        std::cerr << "[!] Error al codificar SLIP\n";
        return;
    }

    // Enviar por UART
    std::string mensaje_str(slip_bytes.begin(), slip_bytes.end());
    uart.enviar(mensaje_str);

    std::cout << "[✓] Hello enviado correctamente.\n";
}

void Nodo::enviarHello()
{
    std::cout << "Enviando mensaje 'Hello' a todos los Nodos cercanos..." << std::endl;
}

void Nodo::run()
{
    std::cout << "Iniciando el nodo..." << std::endl;
    this->menu();
    std::cout << "Nodo finalizado." << std::endl;
    menu();
}

void Nodo::menu()
{
    short opcion;
    ComunicacionUART comunicacion("/dev/ttyUSB0", 115200);
    if (!comunicacion.abrir())
    {
        std::cerr << "No se pudo abrir la comunicación UART." << std::endl;
        return;
    }

    do
    {
        actualizarMensajesEntrantes();
        std::cout << "===============MENU===============:\n";
        std::cout << "1. Ver nodos disponibles\n";
        std::cout << "2. Enviar mensaje Hello\n";
        std::cout << "3. Salir\n";
        std::cin >> opcion;

        switch (opcion)
        {
        case 1:
            this->verNodos();
            break;
        case 2:
            this->enviarHello();
            break;
        case 3:
            break;
        default:
            std::cout << "Opción no válida. Intente de nuevo." << std::endl;
        }
    } while (opcion != 3);
}