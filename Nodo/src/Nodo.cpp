#include "Nodo.h"
#include "ComunicacionUART.h"
#include "Slip.h"
#include "IPv4.h"
#include "PropioProtocolo.h"
#include <iostream>
#include <map>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <unistd.h>

#include <termios.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>

Nodo::Nodo(uint16_t ip) : uart("/dev/ttyUSB0", 115200), ip_nodo(ip), contador_id(1)
{
    if (!uart.abrir())
    {
        std::cerr << "Error: No se pudo abrir el puerto UART" << std::endl;
    }
}

Nodo::~Nodo()
{
    uart.cerrar();
}

uint16_t Nodo::obtenerNuevoID()
{
    return contador_id++;
}

void Nodo::limpiarPantalla()
{
    system("clear");
}

void Nodo::actualizarMensajesEntrantes()
{
    ByteVector datos_recibidos = uart.recibir();

    if (datos_recibidos.empty())
        return;

    ByteVector desempaquetado;
    if (!SLIP_decode(datos_recibidos, desempaquetado))
    {
        std::cerr << "[!] Error al decodificar SLIP" << std::endl;
        return;
    }

    IPv4 paquete;
    if (!parsearIPv4(desempaquetado, paquete))
    {
        std::cerr << "[!] Error al parsear IPv4" << std::endl;
        return;
    }

    // Verificar que el paquete esté dirigido a este nodo o sea broadcast
    if (paquete.ip_destino != ip_nodo && paquete.ip_destino != 0xFFFF)
    {
        return; // No es para este nodo
    }

    // Procesar diferentes tipos de mensajes según protocolo
    switch (paquete.protocolo)
    {
    case 1: // ACK
        procesarACK(paquete);
        break;
    case 2: // Mensaje Unicast
        procesarMensajeUnicast(paquete);
        break;
    case 3: // Mensaje Broadcast
        procesarMensajeBroadcast(paquete);
        break;
    case 4: // Hello
        procesarHello(paquete);
        break;
    case 5: // Prueba
        procesarComandoPrueba(paquete);
        break;
    case 6: // Estado Led
        procesarComandoLed(paquete);
        break;
    case 7: // Mensaje en OLED
        procesarComandoOLED(paquete);
        break;
    default:
        std::cout << "[!] Protocolo desconocido: " << (int)paquete.protocolo << std::endl;
        break;
    }

    verificarACKsPendientes();
}

void Nodo::procesarACK(const IPv4 &paquete)
{
    if (paquete.datos.size() >= 2)
    {
        uint16_t id_confirmado = (paquete.datos[0] << 8) | paquete.datos[1];
        std::cout << "[+] ACK recibido de nodo 0x" << std::hex << paquete.ip_origen
                  << " para mensaje ID: " << id_confirmado << std::dec << std::endl;

        // Buscar y eliminar el ACK pendiente
        std::map<uint16_t, ACKPendiente>::iterator it = acksEsperando.find(id_confirmado);
        if (it != acksEsperando.end())
        {
            acksEsperando.erase(it);
        }
    }
}

void Nodo::procesarMensajeUnicast(const IPv4 &paquete)
{
    std::cout << "[+] Mensaje unicast de nodo 0x" << std::hex << paquete.ip_origen << std::dec << ": ";
    std::cout << std::string(paquete.datos.begin(), paquete.datos.end()) << std::endl;

    // Enviar ACK
    enviarACK(paquete.ip_origen, paquete.identificador);
}

void Nodo::procesarMensajeBroadcast(const IPv4 &paquete)
{
    std::cout << "[+] Mensaje broadcast de nodo 0x" << std::hex << paquete.ip_origen << std::dec << ": ";
    std::cout << std::string(paquete.datos.begin(), paquete.datos.end()) << std::endl;
}

void Nodo::procesarHello(const IPv4 &paquete)
{
    time_t ahora = time(NULL);
    tablaNodosHello[paquete.ip_origen] = ahora;
    // No mostrar mensaje Hello según requisitos
}

void Nodo::procesarComandoPrueba(const IPv4 &paquete)
{
    std::cout << "[+] Comando de prueba recibido de nodo 0x" << std::hex << paquete.ip_origen << std::dec << std::endl;

    // Enviar comando al modem para mostrar imagen de prueba
    PropioProtocolo comando;
    comando.cmd = 5; // Comando de prueba
    comando.longitud_de_dato = 0;
    comando.fcs = calcularFCS(comando);
    enviarComandoAlModem(comando);

    enviarACK(paquete.ip_origen, paquete.identificador);
}

void Nodo::procesarComandoLed(const IPv4 &paquete)
{
    std::cout << "[+] Comando LED recibido de nodo 0x" << std::hex << paquete.ip_origen << std::dec << std::endl;

    // Enviar comando al modem para cambiar estado del LED
    PropioProtocolo comando;
    comando.cmd = 6; // Comando LED
    comando.longitud_de_dato = 0;
    comando.fcs = calcularFCS(comando);
    enviarComandoAlModem(comando);

    enviarACK(paquete.ip_origen, paquete.identificador);
}

void Nodo::procesarComandoOLED(const IPv4 &paquete)
{
    std::cout << "[+] Comando OLED recibido de nodo 0x" << std::hex << paquete.ip_origen << std::dec << ": ";
    std::cout << std::string(paquete.datos.begin(), paquete.datos.end()) << std::endl;

    // Enviar comando al modem para mostrar mensaje en OLED
    PropioProtocolo comando;
    comando.cmd = 7; // Comando OLED
    comando.longitud_de_dato = paquete.datos.size();

    // Copiar datos (máximo 63 bytes)
    size_t bytes_a_copiar = (paquete.datos.size() > 63) ? 63 : paquete.datos.size();
    for (size_t i = 0; i < bytes_a_copiar; i++)
    {
        comando.dato[i] = paquete.datos[i];
    }
    comando.fcs = calcularFCS(comando);

    enviarComandoAlModem(comando);
    enviarACK(paquete.ip_origen, paquete.identificador);
}

void Nodo::enviarACK(uint16_t ip_destino, uint16_t id_mensaje)
{
    IPv4 paquete;
    paquete.flag_fragmento = 0;
    paquete.offset_fragmento = 0;
    paquete.longitud_total = 2;               // 2 bytes para el ID
    paquete.identificador = obtenerNuevoID(); // ID único
    paquete.protocolo = 1;                    // ACK
    paquete.ip_origen = ip_nodo;
    paquete.ip_destino = ip_destino;

    // Convertir ID a bytes
    paquete.datos.push_back((id_mensaje >> 8) & 0xFF);
    paquete.datos.push_back(id_mensaje & 0xFF);

    // Calcular checksum
    paquete.checksum = calcularChecksum(paquete);

    enviarPaquete(paquete);
}

void Nodo::enviarComandoAlModem(const PropioProtocolo &comando)
{
    IPv4 paquete;
    paquete.flag_fragmento = 0;
    paquete.offset_fragmento = 0;
    paquete.longitud_total = 2 + comando.longitud_de_dato; // cmd + longitud + datos
    paquete.identificador = obtenerNuevoID();
    paquete.protocolo = 0; // Protocolo propio
    paquete.ip_origen = ip_nodo;
    paquete.ip_destino = ip_nodo; // Mismo nodo para protocolo propio

    // Construir datos del protocolo propio
    paquete.datos.push_back(comando.cmd);
    paquete.datos.push_back(comando.longitud_de_dato);
    for (int i = 0; i < comando.longitud_de_dato; i++)
    {
        paquete.datos.push_back(comando.dato[i]);
    }

    // Calcular checksum
    paquete.checksum = calcularChecksum(paquete);

    enviarPaquete(paquete);
}

void Nodo::enviarPaquete(const IPv4 &paquete)
{
    ByteVector ipv4_bytes = construirIPv4(paquete);
    ByteVector slip_bytes;

    if (!SLIP_encode(ipv4_bytes, slip_bytes))
    {
        std::cerr << "[!] Error al codificar SLIP" << std::endl;
        return;
    }

    int bytes_enviados = uart.enviar(slip_bytes);
    if (bytes_enviados < 0)
    {
        std::cerr << "[!] Error al enviar por UART" << std::endl;
    }
}

void Nodo::verificarACKsPendientes()
{
    time_t ahora = time(NULL);
    std::vector<uint16_t> ids_a_eliminar;

    for (std::map<uint16_t, ACKPendiente>::iterator it = acksEsperando.begin(); it != acksEsperando.end(); ++it)
    {
        ACKPendiente &ack = it->second;

        if (difftime(ahora, ack.tiempo_envio) >= 3.0)
        {
            if (ack.intentos < 1)
            {
                std::cout << "[!] Reintentando envío de ID " << ack.id_mensaje << " a nodo 0x"
                          << std::hex << ack.ip_destino << std::dec << std::endl;

                // reenviar el paquete correspondiente (debes tener una copia de cada paquete si quieres reenviarlo)
                // O puedes reconstruirlo si sabes qué tipo era.

                ack.intentos++;
                ack.tiempo_envio = ahora;
            }
            else
            {
                std::cout << "[!] No se recibió ACK para ID " << ack.id_mensaje
                          << " después de 2 intentos. Descartando." << std::endl;
                ids_a_eliminar.push_back(it->first);
            }
        }
    }

    for (size_t i = 0; i < ids_a_eliminar.size(); ++i)
    {
        acksEsperando.erase(ids_a_eliminar[i]);
    }
}

void Nodo::verNodos()
{
    std::cout << "\n=============== NODOS DISPONIBLES ===============" << std::endl;

    if (tablaNodosHello.empty())
    {
        std::cout << "No se han recibido mensajes Hello de ningún nodo." << std::endl;
        return;
    }

    time_t ahora = time(NULL);

    std::cout << "IP Nodo\t\tTiempo transcurrido" << std::endl;
    std::cout << "-------\t\t-------------------" << std::endl;

    for (std::map<uint16_t, time_t>::iterator it = tablaNodosHello.begin();
         it != tablaNodosHello.end(); ++it)
    {
        uint16_t ip = it->first;
        time_t recibido = it->second;

        double segundos = difftime(ahora, recibido);

        std::cout << "0x" << std::hex << ip << std::dec << "\t\t"
                  << (int)segundos << " segundos" << std::endl;
    }
    std::cout << "=================================================" << std::endl;
}

void Nodo::enviarHello()
{
    std::cout << "[+] Enviando mensaje Hello..." << std::endl;

    IPv4 paquete;
    paquete.flag_fragmento = 0;
    paquete.offset_fragmento = 0;
    paquete.longitud_total = 4; // "hola"
    paquete.identificador = obtenerNuevoID();
    paquete.protocolo = 4; // Hello
    paquete.ip_origen = ip_nodo;
    paquete.ip_destino = 0xFFFF; // Broadcast

    // Mensaje "hola"
    std::string mensaje = "hola";
    for (size_t i = 0; i < mensaje.length(); i++)
    {
        paquete.datos.push_back(mensaje[i]);
    }

    // Calcular checksum
    paquete.checksum = calcularChecksum(paquete);

    enviarPaquete(paquete);
    std::cout << "[✓] Hello enviado correctamente." << std::endl;
}

void Nodo::enviarMensajeUnicast()
{
    std::string buffer;
    uint16_t ip_destino = 0;

    std::cout << "Ingrese IP destino (en hexadecimal, ej: 10 para 0x0010): ";
    std::cout.flush();
    buffer.clear();

    while (!leerLineaNoBloqueante(buffer))
    {
        actualizarMensajesEntrantes();
        usleep(50000);
    }

    std::stringstream ss(buffer);
    ss >> std::hex >> ip_destino;

    if (tablaNodosHello.find(ip_destino) == tablaNodosHello.end())
    {
        std::cout << "[!] Nodo 0x" << std::hex << ip_destino << std::dec
                  << " no está disponible. Envíe un Hello primero." << std::endl;
        return;
    }

    std::cout << "Ingrese el mensaje: ";
    std::cout.flush();
    buffer.clear();

    while (!leerLineaNoBloqueante(buffer))
    {
        actualizarMensajesEntrantes();
        usleep(50000);
    }

    std::string mensaje = buffer;

    if (mensaje.empty())
    {
        std::cout << "[!] El mensaje no puede estar vacío." << std::endl;
        return;
    }

    IPv4 paquete;
    paquete.flag_fragmento = 0;
    paquete.offset_fragmento = 0;
    paquete.longitud_total = mensaje.length();
    paquete.identificador = obtenerNuevoID();
    paquete.protocolo = 2; // Mensaje Unicast
    paquete.ip_origen = ip_nodo;
    paquete.ip_destino = ip_destino;

    for (size_t i = 0; i < mensaje.length(); i++)
    {
        paquete.datos.push_back(mensaje[i]);
    }

    paquete.checksum = calcularChecksum(paquete);

    // Agregar a ACKs esperando (el reintento lo maneja verificarACKsPendientes)
    ACKPendiente ack;
    ack.ip_destino = ip_destino;
    ack.id_mensaje = paquete.identificador;
    ack.intentos = 0;
    ack.tiempo_envio = time(NULL);
    acksEsperando[paquete.identificador] = ack;

    enviarPaquete(paquete);

    std::cout << "[✓] Mensaje enviado a nodo 0x" << std::hex << ip_destino << std::dec << std::endl;
    std::cout << "[...] Esperando ACK en segundo plano...\n";
}

void Nodo::enviarMensajeBroadcast()
{
    std::string buffer;
    std::cout << "Ingrese el mensaje broadcast: ";
    std::cout.flush();
    buffer.clear();

    while (!leerLineaNoBloqueante(buffer))
    {
        actualizarMensajesEntrantes();
        usleep(50000);
    }

    std::string mensaje = buffer;

    if (mensaje.empty())
    {
        std::cout << "[!] El mensaje no puede estar vacío." << std::endl;
        return;
    }

    IPv4 paquete;
    paquete.flag_fragmento = 0;
    paquete.offset_fragmento = 0;
    paquete.longitud_total = mensaje.length();
    paquete.identificador = obtenerNuevoID();
    paquete.protocolo = 3; // Broadcast
    paquete.ip_origen = ip_nodo;
    paquete.ip_destino = 0xFFFF;

    for (size_t i = 0; i < mensaje.length(); i++)
    {
        paquete.datos.push_back(mensaje[i]);
    }

    paquete.checksum = calcularChecksum(paquete);

    enviarPaquete(paquete);
    std::cout << "[✓] Mensaje broadcast enviado." << std::endl;
}

void Nodo::enviarComandoPrueba()
{
    std::string buffer;
    uint16_t ip_destino;

    std::cout << "Ingrese IP destino (en hexadecimal): ";
    std::cout.flush();
    buffer.clear();

    while (!leerLineaNoBloqueante(buffer))
    {
        actualizarMensajesEntrantes();
        usleep(50000);
    }

    std::stringstream ss(buffer);
    ss >> std::hex >> ip_destino;

    if (tablaNodosHello.find(ip_destino) == tablaNodosHello.end())
    {
        std::cout << "[!] Nodo 0x" << std::hex << ip_destino << std::dec << " no está disponible." << std::endl;
        return;
    }

    IPv4 paquete;
    paquete.flag_fragmento = 0;
    paquete.offset_fragmento = 0;
    paquete.longitud_total = 0;
    paquete.identificador = obtenerNuevoID();
    paquete.protocolo = 5; // Prueba
    paquete.ip_origen = ip_nodo;
    paquete.ip_destino = ip_destino;

    paquete.checksum = calcularChecksum(paquete);

    ACKPendiente ack;
    ack.ip_destino = ip_destino;
    ack.id_mensaje = paquete.identificador;
    ack.intentos = 0;
    ack.tiempo_envio = time(NULL);
    acksEsperando[paquete.identificador] = ack;

    enviarPaquete(paquete);
    std::cout << "[✓] Comando de prueba enviado. Esperando ACK en segundo plano...\n";
}

void Nodo::enviarComandoLed()
{
    std::string buffer;
    uint16_t ip_destino;

    std::cout << "Ingrese IP destino (en hexadecimal): ";
    std::cout.flush();
    buffer.clear();

    while (!leerLineaNoBloqueante(buffer))
    {
        actualizarMensajesEntrantes();
        usleep(50000);
    }

    std::stringstream ss(buffer);
    ss >> std::hex >> ip_destino;

    if (tablaNodosHello.find(ip_destino) == tablaNodosHello.end())
    {
        std::cout << "[!] Nodo 0x" << std::hex << ip_destino << std::dec << " no está disponible." << std::endl;
        return;
    }

    IPv4 paquete;
    paquete.flag_fragmento = 0;
    paquete.offset_fragmento = 0;
    paquete.longitud_total = 0;
    paquete.identificador = obtenerNuevoID();
    paquete.protocolo = 6; // LED
    paquete.ip_origen = ip_nodo;
    paquete.ip_destino = ip_destino;

    paquete.checksum = calcularChecksum(paquete);

    ACKPendiente ack;
    ack.ip_destino = ip_destino;
    ack.id_mensaje = paquete.identificador;
    ack.intentos = 0;
    ack.tiempo_envio = time(NULL);
    acksEsperando[paquete.identificador] = ack;

    enviarPaquete(paquete);
    std::cout << "[✓] Comando LED enviado. Esperando ACK en segundo plano...\n";
}

void Nodo::enviarMensajeOLED()
{
    std::string buffer;
    uint16_t ip_destino;

    std::cout << "Ingrese IP destino (en hexadecimal): ";
    std::cout.flush();
    buffer.clear();

    while (!leerLineaNoBloqueante(buffer))
    {
        actualizarMensajesEntrantes();
        usleep(50000);
    }

    std::stringstream ss(buffer);
    ss >> std::hex >> ip_destino;

    if (tablaNodosHello.find(ip_destino) == tablaNodosHello.end())
    {
        std::cout << "[!] Nodo 0x" << std::hex << ip_destino << std::dec << " no está disponible." << std::endl;
        return;
    }

    std::cout << "Ingrese mensaje para OLED: ";
    std::cout.flush();
    buffer.clear();

    while (!leerLineaNoBloqueante(buffer))
    {
        actualizarMensajesEntrantes();
        usleep(50000);
    }

    std::string mensaje = buffer;

    if (mensaje.empty())
    {
        std::cout << "[!] El mensaje no puede estar vacío." << std::endl;
        return;
    }

    IPv4 paquete;
    paquete.flag_fragmento = 0;
    paquete.offset_fragmento = 0;
    paquete.longitud_total = mensaje.length();
    paquete.identificador = obtenerNuevoID();
    paquete.protocolo = 7; // OLED
    paquete.ip_origen = ip_nodo;
    paquete.ip_destino = ip_destino;

    for (size_t i = 0; i < mensaje.length(); i++)
    {
        paquete.datos.push_back(mensaje[i]);
    }

    paquete.checksum = calcularChecksum(paquete);

    ACKPendiente ack;
    ack.ip_destino = ip_destino;
    ack.id_mensaje = paquete.identificador;
    ack.intentos = 0;
    ack.tiempo_envio = time(NULL);
    acksEsperando[paquete.identificador] = ack;

    enviarPaquete(paquete);
    std::cout << "[✓] Mensaje OLED enviado. Esperando ACK en segundo plano...\n";
}

void Nodo::menuComandosInternos()
{
    std::string buffer;
    int opcion = -1;

    do
    {
        actualizarMensajesEntrantes();

        std::cout << "\n========== COMANDOS INTERNOS ==========" << std::endl;
        std::cout << "1. Comando de prueba" << std::endl;
        std::cout << "2. Cambiar estado LED" << std::endl;
        std::cout << "3. Enviar mensaje a OLED" << std::endl;
        std::cout << "4. Volver al menú principal" << std::endl;
        std::cout << "Seleccione una opción: ";
        std::cout.flush();

        buffer.clear();
        while (!leerLineaNoBloqueante(buffer))
        {
            actualizarMensajesEntrantes();
            usleep(50000);
        }

        opcion = atoi(buffer.c_str());

        switch (opcion)
        {
        case 1:
            enviarComandoPrueba();
            break;
        case 2:
            enviarComandoLed();
            break;
        case 3:
            enviarMensajeOLED();
            break;
        case 4:
            std::cout << "Volviendo al menú principal..." << std::endl;
            break;
        default:
            std::cout << "[!] Opción no válida." << std::endl;
            break;
        }

    } while (opcion != 4);
}

void Nodo::menuEnvioMensajes()
{
    std::string buffer;
    int opcion = -1;

    do
    {
        actualizarMensajesEntrantes();

        std::cout << "\n========== ENVÍO DE MENSAJES ==========" << std::endl;
        std::cout << "1. Enviar mensaje unicast" << std::endl;
        std::cout << "2. Enviar mensaje broadcast" << std::endl;
        std::cout << "3. Volver al menú principal" << std::endl;
        std::cout << "Seleccione una opción: ";
        std::cout.flush();

        buffer.clear();
        while (!leerLineaNoBloqueante(buffer))
        {
            actualizarMensajesEntrantes();
            usleep(50000);
        }

        opcion = atoi(buffer.c_str());

        switch (opcion)
        {
        case 1:
            enviarMensajeUnicast();
            break;
        case 2:
            enviarMensajeBroadcast();
            break;
        case 3:
            std::cout << "Volviendo al menú principal..." << std::endl;
            break;
        default:
            std::cout << "[!] Opción no válida." << std::endl;
            break;
        }

    } while (opcion != 3);
}

void Nodo::menu()
{
    configurarEntradaNoBloqueante(); // Activar entrada no bloqueante
    std::string buffer;
    int opcion = -1;

    srand(time(NULL));

    do
    {
        actualizarMensajesEntrantes();

        std::cout << "\r=============== MENÚ PRINCIPAL ===============\n";
        std::cout << "IP del nodo: 0x" << std::hex << ip_nodo << std::dec << "\n";
        std::cout << "1. Ver nodos disponibles\n";
        std::cout << "2. Enviar mensaje Hello\n";
        std::cout << "3. Comandos internos del modem\n";
        std::cout << "4. Enviar mensajes a otros nodos\n";
        std::cout << "5. Salir\n";
        std::cout << "Seleccione una opción: ";
        std::cout.flush();

        buffer.clear();
        while (!leerLineaNoBloqueante(buffer))
        {
            actualizarMensajesEntrantes(); // sigue escuchando UART
            usleep(50000);
        }

        opcion = atoi(buffer.c_str());

        switch (opcion)
        {
        case 1:
            verNodos();
            break;
        case 2:
            enviarHello();
            break;
        case 3:
            menuComandosInternos();
            break;
        case 4:
            menuEnvioMensajes();
            break;
        case 5:
            std::cout << "Saliendo...\n";
            break;
        default:
            std::cout << "[!] Opción inválida\n";
            break;
        }

    } while (opcion != 5);

    restaurarEntradaOriginal(); // Restaurar terminal
}

void Nodo::configurarEntradaNoBloqueante()
{
    struct termios tty;
    tcgetattr(STDIN_FILENO, &tty);

    tty.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &tty);

    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
}

void Nodo::restaurarEntradaOriginal()
{
    struct termios tty;
    tcgetattr(STDIN_FILENO, &tty);
    tty.c_lflag |= (ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &tty);

    fcntl(STDIN_FILENO, F_SETFL, 0);
}

// Leer línea no bloqueante
bool Nodo::leerLineaNoBloqueante(std::string &buffer)
{
    char c;
    while (read(STDIN_FILENO, &c, 1) > 0)
    {
        if (c == '\n')
        {
            return true;
        }
        buffer += c;
    }
    return false;
}

void Nodo::run()
{
    std::cout << "=== INICIANDO NODO LoRa ===" << std::endl;
    std::cout << "IP del nodo: 0x" << std::hex << ip_nodo << std::dec << std::endl;

    if (!uart.estaAbierto())
    {
        std::cerr << "Error: No se pudo establecer comunicación UART" << std::endl;
        return;
    }

    std::cout << "Comunicación UART establecida correctamente" << std::endl;

    menu();

    std::cout << "=== NODO FINALIZADO ===" << std::endl;
}