#include "ComunicacionUART.h"
#include <iostream>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <cstring>
#include <errno.h>

ComunicacionUART::ComunicacionUART(const std::string &dispositivo, int baudios)
    : dispositivo_(dispositivo), baudios_(baudios), descriptor_(-1), abierto_(false) {}

ComunicacionUART::~ComunicacionUART()
{
    cerrar();
}

bool ComunicacionUART::abrir()
{
    descriptor_ = open(dispositivo_.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (descriptor_ < 0)
    {
        std::cerr << "Error al abrir el puerto " << dispositivo_ << std::endl;
        return false;
    }

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

int ComunicacionUART::enviar(const ByteVector &mensaje)
{
    if (!abierto_)
        return -1;
    return write(descriptor_, &mensaje[0], mensaje.size());
}

ByteVector ComunicacionUART::recibir()
{
    ByteVector resultado;

    if (!abierto_)
        return resultado;

    uint8_t buffer[256];
    int n = read(descriptor_, buffer, sizeof(buffer));

    if (n > 0)
    {
        resultado.assign(buffer, buffer + n);
    }
    else if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
    {
        std::cerr << "Error de lectura UART" << std::endl;
    }

    return resultado;
}
