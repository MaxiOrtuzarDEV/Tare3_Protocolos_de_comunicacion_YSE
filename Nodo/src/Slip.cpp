#include "include/Slip.h"

#include "SLIP.h"

#define SLIP_END 0xC0
#define SLIP_ESC 0xDB
#define SLIP_ESC_END 0xDC
#define SLIP_ESC_ESC 0xDD

bool SLIP_encode(const ByteVector &entrada, ByteVector &salida)
{
    salida.clear();
    salida.push_back(SLIP_END);

    for (BYTE b : entrada)
    {
        if (b == SLIP_END)
        {
            salida.push_back(SLIP_ESC);
            salida.push_back(SLIP_ESC_END);
        }
        else if (b == SLIP_ESC)
        {
            salida.push_back(SLIP_ESC);
            salida.push_back(SLIP_ESC_ESC);
        }
        else
        {
            salida.push_back(b);
        }
    }

    salida.push_back(SLIP_END);
    return true;
}

bool SLIP_decode(const ByteVector &entrada, ByteVector &salida)
{
    salida.clear();
    bool dentro = false;

    for (size_t i = 0; i < entrada.size(); ++i)
    {
        BYTE b = entrada[i];

        if (b == SLIP_END)
        {
            if (dentro && !siguiente_es_fin(entrada, i))
                return true;
            dentro = true;
            continue;
        }

        if (!dentro)
            continue;

        if (b == SLIP_ESC)
        {
            if (i + 1 >= entrada.size())
                return false;
            BYTE siguiente = entrada[++i];
            if (siguiente == SLIP_ESC_END)
                salida.push_back(SLIP_END);
            else if (siguiente == SLIP_ESC_ESC)
                salida.push_back(SLIP_ESC);
            else
                return false;
        }
        else
        {
            salida.push_back(b);
        }
    }

    return !salida.empty();
}

bool siguiente_es_fin(const ByteVector &entrada, size_t desde)
{
    for (size_t j = desde + 1; j < entrada.size(); ++j)
    {
        if (entrada[j] == SLIP_END)
            return true;
        if (entrada[j] != 0x00)
            return false;
    }
    return false;
}
