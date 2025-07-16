#include "red.h"
#define TAM_DATOS_LORA 255
volatile bool flag_rxDone = false; // Recepción completada
volatile BYTE buffer_rx[TAM_DATOS_LORA]; // Buffer de recepción LoRa
volatile int last_tamDato; // tamaño del último dato recibido.

void IRAM_ATTR recepcion(int tamDato){
    if (tamDato==0 || tamDato > TAM_DATOS_LORA) return;    // tamaño incorrecto?
    LoRa.readPayload((BYTE *)buffer_rx,tamDato);        // se lee el payload
    last_tamDato = tamDato;
    flag_rxDone = true;
}

Red::Red()
{

}

void Red::transmite_data(BYTE * data,BYTE largo,bool rx_on){
    LoRa.beginPacket();
    LoRa.write(data,largo);
    LoRa.endPacket();
    if (rx_on) lora_sleep(false);
}

void Red::lora_sleep(bool sleep){
    if (sleep){
        LoRa.sleep();// Modo Sleep (la radio queda en modo bajo consumo)
    }
    else{
        LoRa.receive();// Modo Recepción Continua
    }
}

void Red::begin(int sf,long bw,int CR,int txpwr){
    SPI.begin(PIN_CLK_SPI_LORA,PIN_MISO_SPI_LORA,PIN_MOSI_SPI_LORA);// pines SPI segun esquematico Heltec
    //https://resource.heltec.cn/download/WiFi_LoRa_32/V2/WIFI_LoRa_32_V2(868-915).PDF
    do
    {
        if (LoRa.begin(915E6))break;// intenta inicializar la radio LoRa
        Serial.println("No se pudo iniciar radio LoRa");
        delay(1000); // en caso de error se espera 1 segundo
    } while (1);
    
    LoRa.enableCrc();                        // habilita el CRC en la data
    LoRa.setPreambleLength(PREAMBLE_LENGTH); // largo del preambulo
    setconfLoRa(sf,bw,CR,txpwr);             // asignación de parámetros
    LoRa.onReceive(recepcion);               // preparar interrupción
    lora_sleep(false);
}

void Red::setconfLoRa(int sf,long bw,int CR,int txpwr)
{   
    _sf = sf; // factor de ensanchamiento
    _bw = bw; // ancho de banda
    _crc = CR; // factor de codificación
    LoRa.setSpreadingFactor(sf);
    LoRa.setSignalBandwidth(bw);
    LoRa.setCodingRate4(CR);
    LoRa.setTxPower(txpwr);
}


bool  Red::dataDisponible(){
    return flag_rxDone;
}

int Red::getData(BYTE *data,int n){
    if (flag_rxDone)
    {
        n = min(n,(int)last_tamDato);
        memcpy(data,(BYTE *)buffer_rx,(int)last_tamDato);
        flag_rxDone = false;
        return n;
    }
    return 0;// No hay datos.
}
