#include "include/Nodo.h"
#include <iostream>
#include <cstdlib>

int main(int argc, char *argv[])
{
    uint16_t ip_nodo = 0x0001; // IP por defecto

    // Permitir especificar IP como argumento
    if (argc > 1)
    {
        ip_nodo = (uint16_t)strtol(argv[1], NULL, 16);
    }

    std::cout << "Iniciando nodo con IP: 0x" << std::hex << ip_nodo << std::dec << std::endl;

    Nodo nodo(ip_nodo);
    nodo.run();

    return 0;
}