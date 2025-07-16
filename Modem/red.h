#ifndef RED_H
#define RED_H
#include <SPI.h>
#include "LoRa.h"
#include <Arduino.h>

// Pines SPI LoRa
#define PIN_CLK_SPI_LORA  5     // pin CLK
#define PIN_MISO_SPI_LORA 19    // pin MISO SPI 
#define PIN_MOSI_SPI_LORA 27    // pin MOSI SPI
// extras
#define PREAMBLE_LENGTH     12       // tamaño del preambulo LoRa
#define BYTE unsigned char
/**
 * @brief Recepción
 * Función para manejar interrupción de recepción LoRa
 * 
 * @param tamDato tamaño del dato LoRa obtenido desde la radio
 */
void recepcion(int tamDato);

class Red
{
private:
    bool flg_rxOK = false;


    void setconfLoRa(int sf,long bw,int CR=1,int txpwr=2);
    /**
     * @brief dormir radio LoRa
     * 
     * @param sleep dormir lora?
     */
    void lora_sleep(bool sleep);
public:
    Red();// constructor

    //parametros LoRa 
    int _sf; // Spreading Factor
    long _bw; // Ancho de banda
    int _crc; // Factor de codificación para detección de errores
    void begin(int sf=7,long bw=250E3,int CR=1,int txpwr=2);
    
    bool dataDisponible();// dato listo para extración
    int getData(BYTE *data,int n); // Obtener datos disponibles
    /**
     * @brief transmitir datos por lora
     * 
     * @param data buffer a transmitir < 256 bytes
     * @param largo largo del buffer
     * @param rx_on activar modo recepción luego del envio
     */
    void transmite_data(BYTE * data,BYTE largo,bool rx_on=true);
};
#endif