#ifndef LORAWAN_HPP
#define LORAWAN_HPP

#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <Arduino.h>

#define SS 5
#define RST 14
#define DIO0 26
#define CFG_sx1276_radio 1

static const u1_t PROGMEM APPEUI[8] = {0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01};
void os_getArtEui(u1_t *buf) { memcpy_P(buf, APPEUI, 8); }

static const u1_t PROGMEM DEVEUI[8] = {0x17, 0x4B, 0xD4, 0xD4, 0xC6, 0xF9, 0x29, 0xB0};
void os_getDevEui(u1_t *buf) { memcpy_P(buf, DEVEUI, 8); }

static const u1_t PROGMEM APPKEY[16] = {0xC8, 0x7B, 0x8A, 0x6A, 0x51, 0x62, 0x99, 0xB4, 0x5F, 0x9C, 0x44, 0x28, 0xC3, 0x66, 0x43, 0xC2};
void os_getDevKey(u1_t *buf) { memcpy_P(buf, APPKEY, 16); }

class LoRaWanConnection
{
private:
    SPIClass SPI;
    osjob_t sendJob;

    const unsigned TX_INTERVAL = 60;
    
public:
    LoRaWanConnection() : SPI(SPIClass())
    {
        this->SPI.begin(18, 19, 23, 5);
        this->sendJob = osjob_t();

    }
};

#endif
