#include "Slip.h"

bool SLIP_encode(const ByteVector &entrada, ByteVector &salida)
{
    salida.clear();
    salida.push_back(SLIP_END);

    for (ByteVector::const_iterator it = entrada.begin(); it != entrada.end(); ++it)
    {
        BYTE b = *it;
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
            if (dentro && !salida.empty())
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