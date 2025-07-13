#include "include/ComunicacionUART.h"
#include <iostream>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <cstring>

ComunicacionUART::ComunicacionUART(const std::string &dispositivo, int baudios)
    : dispositivo_(dispositivo), baudios_(baudios), descriptor_(-1), abierto_(false) {}

ComunicacionUART::~ComunicacionUART()
{
    cerrar();
}

bool ComunicacionUART::abrir()
{
    descriptor_ = open(dispositivo_.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
    if (descriptor_ < 0)
    {
        std::cerr << "Error al abrir el puerto " << dispositivo_ << std::endl;
        return false;
    }

    fcntl(descriptor_, F_SETFL, 0); // modo bloqueante

    struct termios opciones;
    tcgetattr(descriptor_, &opciones);

    cfsetispeed(&opciones, B115200);
    cfsetospeed(&opciones, B115200);

    opciones.c_cflag &= ~PARENB;
    opciones.c_cflag &= ~CSTOPB;
    opciones.c_cflag &= ~CSIZE;
    opciones.c_cflag |= CS8;
    opciones.c_cflag |= CREAD | CLOCAL;

    opciones.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    opciones.c_iflag &= ~(IXON | IXOFF | IXANY);
    opciones.c_oflag &= ~OPOST;

    tcsetattr(descriptor_, TCSANOW, &opciones);

    abierto_ = true;
    return true;
}

void ComunicacionUART::cerrar()
{
    if (abierto_)
    {
        close(descriptor_);
        abierto_ = false;
    }
}

bool ComunicacionUART::estaAbierto() const
{
    return abierto_;
}

int ComunicacionUART::enviar(const std::string &mensaje)
{
    if (!abierto_)
        return -1;
    return write(descriptor_, mensaje.c_str(), mensaje.size());
}

std::string ComunicacionUART::recibir(int maxLongitud)
{
    if (!abierto_)
        return "";

    char buffer[256];
    memset(buffer, 0, sizeof(buffer));
    int n = read(descriptor_, buffer, maxLongitud > 256 ? 256 : maxLongitud);
    if (n > 0)
        return std::string(buffer, n);
    return "";
}
