#include <arduino_lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <SoftwareSerial.h>
#include <Arduino.h>
static const u1_t PROGMEM APPSKEY[16] = {
    0xC8, 0x7B, 0x8A, 0x6A, 0x51, 0x62, 0x99, 0xB4, 0x5F, 0x9C, 0x44, 0x28, 0xC3, 0x66, 0x43, 0xC2};

// [Network Session Key] - Chave da Rede (big-endian)
static const u1_t PROGMEM NWKSKEY[16] = {
    0xDF, 0x9D, 0xBD, 0x9E, 0xC0, 0xC8, 0x3A, 0xF6, 0x82, 0x53, 0x58, 0x47, 0x2F, 0x74, 0x22, 0x72};

// [Device Address] - Endereço do Dispositivo
static const u4_t DEVADDR = 0xEBA34DB3;

// Protótipo das funções
void os_getArtEui(u1_t *buf) {}
void os_getDevEui(u1_t *buf) {}
void os_getDevKey(u1_t *buf) {}

// Dados para enviar
static uint8_t mydata[] = "jvtech-teste01";

// Instância
static osjob_t sendjob;

// Tempo de envio em milisegundos
const unsigned TX_INTERVAL = 120;

// Pinos
const lmic_pinmap lmic_pins = {

    .nss = 5,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 14,
    .dio = {26, 13, LMIC_UNUSED_PIN}, // (DIO0, DIO1)

};

void do_send(osjob_t *j)
{

  if (LMIC.opmode & OP_TXRXPEND)
  { // Verifica se tem algum processo TX e RX ocorrendo

    Serial.println(F("OP_TXRXPEND, not sending"));
  }
  else
  { // Prepara os dados para envio na próxima janela

    LMIC_setTxData2(1, mydata, sizeof(mydata) - 1, 0);
    Serial.println(F("Packet queued"));
  }
}

// Funções de evento LoRaWAN
void onEvent(ev_t ev)
{

  switch (ev)
  {
  case EV_SCAN_TIMEOUT:
    Serial.println(F("EV_SCAN_TIMEOUT"));
    break;
  case EV_BEACON_FOUND:
    Serial.println(F("EV_BEACON_FOUND"));
    break;
  case EV_BEACON_MISSED:
    Serial.println(F("EV_BEACON_MISSED"));
    break;
  case EV_BEACON_TRACKED:
    Serial.println(F("EV_BEACON_TRACKED"));
    break;
  case EV_JOINING:
    Serial.println(F("EV_JOINING"));
    break;
  case EV_JOINED:
    Serial.println(F("EV_JOINED"));
    break;
  case EV_JOIN_FAILED:
    Serial.println(F("EV_JOIN_FAILED"));
    break;
  case EV_REJOIN_FAILED:
    Serial.println(F("EV_REJOIN_FAILED"));
    break;
  case EV_TXCOMPLETE:
    Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
    if (LMIC.txrxFlags & TXRX_ACK)
      Serial.println(F("Received ack;"));
    if (LMIC.dataLen)
    {
      Serial.print(F("Received "));
      Serial.print(LMIC.dataLen);
      Serial.println(F(" byte(s) of payload."));
    }
    os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(TX_INTERVAL), do_send); // Configuração da próxima transmissão
    break;
  case EV_LOST_TSYNC:
    Serial.println(F("EV_LOST_TSYNC"));
    break;
  case EV_RESET:
    Serial.println(F("EV_RESET"));
    break;
  case EV_RXCOMPLETE:
    Serial.println(F("EV_RXCOMPLETE")); // Dados recebidos
    break;
  case EV_LINK_DEAD:
    Serial.println(F("EV_LINK_DEAD"));
    break;
  case EV_LINK_ALIVE:
    Serial.println(F("EV_LINK_ALIVE"));
    break;
  case EV_TXSTART:
    Serial.println(F("EV_TXSTART"));
    break;
  default:
    Serial.print(F("Unknown event: "));
    Serial.println((unsigned)ev);
    break;
  }
}

void setup()
{

  Serial.begin(115200); // Iniciar serial
  delay(100);
  Serial.println(F("Starting..."));

  // SPI.begin(18, 19, 23, 5); // Iniciar SPI - (SCK,MISO,MOSI,SS)

  os_init();

  LMIC_reset(); // Reinicia rádio LoRa

  uint8_t appskey[sizeof(APPSKEY)];
  uint8_t nwkskey[sizeof(NWKSKEY)];
  memcpy_P(appskey, APPSKEY, sizeof(APPSKEY));
  memcpy_P(nwkskey, NWKSKEY, sizeof(NWKSKEY));
  LMIC_setSession(0x13, DEVADDR, nwkskey, appskey); // Configura LoRaWAN
  //LMIC_selectSubBand(1);

  LMIC_setAdrMode(0);                             // Desabilita o ADR
  LMIC_setLinkCheckMode(0);                       // Desabilita a verificação de validação
  LMIC.dn2Dr = DR_SF9;                            // Configra Spreading Factor para a janela de recepção (Usado para TTN)
  LMIC_setDrTxpow(DR_SF7, 14);                    // Configura o Spreading Factor e potência
  LMIC_setClockError(MAX_CLOCK_ERROR * 10 / 100); // Configura o clock

  do_send(&sendjob); // Inicia os trabalhos
}

void loop()
{

  os_runloop_once(); // Loop de envio
}
