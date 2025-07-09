/*

  Teste LoRaWAN v1.0

  Author: João Vitor Galdino Souto Alvares
  Date: 06/06/2024

  Description: Versão de teste para envio via ABP com a biblioteca LMIC. Essa versão tem a finalidade de validar somente que a comunicação está ok. Não é dispositivo de final na placa principal

          Ativação de segurança			: ABP
          Criptografia de segurança		: NS
          Banda							: AU915-928A
          Classe							: A
          Tamanho do contador				: 2
          Validação de contadores			: Desabilitada

         -> ESP32

          -> With Debug

          This program is utilizing xxxxxx (xx%) bytes of the memory FLASH
          The maximum is of 1310720 (1.3MB) bytes of memory FLASH

          This program is utilizing xxxxxx (xx%) bytes of the memory RAM
          The maximum is of 294912 (294KB) bytes of memory RAM

          -> Without Debug

          This program is utilizing xxxxxx (xx%) bytes of the memory FLASH
          The maximum is of 1310720 (1.3MB) bytes of memory FLASH

          This program is utilizing xxxxxx (xx%) bytes of the memory RAM
          The maximum is of 294912 (294KB) bytes of memory RAM

*/

//*****************************************************************************************************************
// Library(ies)
//*****************************************************************************************************************

#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>

// To LED RGB
#include <Adafruit_NeoPixel.h> // Biblioteca Adafruit_NeoPixel-master.zip na pasta ou versão: 1.9.0 por Adafruit

// To LoRa
#include <SPI.h>  // Biblioteca nativa do ESP32
#include <LoRa.h> // Biblioteca LoRa versão: 0.8.0 por Sandeep Mistry

// To ESP32
#include <Arduino.h> // Biblioteca nativa do ESP32

// To String
#include <stdio.h>  // Biblioteca nativa do ESP32
#include <string.h> // Biblioteca nativa do ESP32
#include <stdlib.h> // Biblioteca nativa do ESP32

// To Math
#include <math.h> // Biblioteca nativa do ESP32

// To JSON
#include <ArduinoJson.h> // To send data JSON

// To I2C
#include <Wire.h> // Biblioteca nativa do ESP32

// To DHT11
#include "DHT.h"

//*****************************************************************************************************************
// Constant(s)
//*****************************************************************************************************************

// Hardware mapping
#define pinToLED 2
#define pinToPushButton 25
#define pinToDHT11 22
#define RADIO_RESET_PORT 14
// #define RADIO_MOSI_PORT      27
#define RADIO_MOSI_PORT 23
#define RADIO_MISO_PORT 19
// #define RADIO_SCLK_PORT      5
#define RADIO_SCLK_PORT 18
// #define RADIO_NSS_PORT       18
#define RADIO_NSS_PORT 5
#define RADIO_DIO_0_PORT 26
#define RADIO_DIO_1_PORT 13

// To RGB
#define NUMPIXELS 1
#define RED pixels1.Color(255, 0, 0)       // Red
#define GREEN pixels1.Color(0, 255, 0)     // Green
#define BLUE pixels1.Color(0, 0, 255)      // Blue
#define WHITE pixels1.Color(255, 255, 255) // White
#define OFF_COLORS pixels1.Color(0, 0, 0)  // OFF
#define MAXCOLORS 5

// Macro(s) generic(s)
// To debug
#define DEBUG

// To serial
#define SERIAL_DEBUG 115200

// To time
#define timeToWait 1

// To LoRaWAN
#define GANHO_LORA_DBM 20

// To DHT11
#define DHTTYPE DHT11

// To JSON
#define MSG_BUFFER_SIZE (100) // Define a quantidade de caracteres da variável msg

//*****************************************************************************************************************
// Prototype of the function(s)
//*****************************************************************************************************************

// To OTAA
void os_getArtEui(u1_t *buf);
void os_getDevEui(u1_t *buf);
void os_getDevKey(u1_t *buf);

// To LoRaWAN
void onEvent(ev_t ev);
void do_send(osjob_t *j);

// To operate
void readDHT11();

//*****************************************************************************************************************
// Object(s)
//*****************************************************************************************************************

// To RGB
Adafruit_NeoPixel pixels1(NUMPIXELS, pinToLED, NEO_GRB + NEO_KHZ800);

// To DHT11
DHT dht(pinToDHT11, DHTTYPE);

//*****************************************************************************************************************
// Global variable(s)
//*****************************************************************************************************************

// To LoRaWAN
// Constantes do rádio LoRa: GPIOs utilizados para comunicação com rádio SX1276
const lmic_pinmap lmic_pins = {
    .nss = RADIO_NSS_PORT,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = RADIO_RESET_PORT,
    .dio = {RADIO_DIO_0_PORT, RADIO_DIO_1_PORT, LMIC_UNUSED_PIN},
};

// Constantes do LoraWAN - com letras maiusculas
// - Chaves (Network Keys)
// deveui: B0-29-F9-C6-D4-D4-4B-17
// appeui: 01-01-01-01-01-01-01-01
// network session key: DF-9D-BD-9E-C0-C8-3A-F6-82-53-58-47-2F-74-22-72
static const PROGMEM u1_t NWKSKEY[16] = {0xDF, 0x9D, 0xBD, 0x9E, 0xC0, 0xC8, 0x3A, 0xF6, 0x82, 0x53, 0x58, 0x47, 0x2F, 0x74, 0x22, 0x72};
// application session key: C8-7B-8A-6A-51-62-99-B4-5F-9C-44-28-C3-66-43-C2
static const u1_t PROGMEM APPSKEY[16] = {0xC8, 0x7B, 0x8A, 0x6A, 0x51, 0x62, 0x99, 0xB4, 0x5F, 0x9C, 0x44, 0x28, 0xC3, 0x66, 0x43, 0xC2};// - Device Address - com letras minusculas
// devaddress: EB-A3-4D-B3
static const u4_t DEVADDR = 0xEBA34DB3; // Teste IOTA
  
// - Tempo entre envios de pacotes LoRa
const unsigned TX_INTERVAL = 1; // 1800s = 30 minutos
// Variáveis e objetos globais
static osjob_t sendjob; // Objeto para job de envio de dados via ABP

// To RGB
uint32_t cores[MAXCOLORS] = {RED, GREEN, BLUE, WHITE, OFF_COLORS};

// To push button
bool flagToPushButton = false;

// To DHT11
float valueTemperature;
float valueHumididty;

// To JSON
char msg[MSG_BUFFER_SIZE] = {0}; // Variável para armazenar o JSON serializado (em formato string)
const int tamanhoDados = 25;

//*****************************************************************************************************************
// Callbacks
//*****************************************************************************************************************

// Callbacks para uso cpm OTAA apenas (por este projeto usar ABP, isso, eles estão vazios)
void os_getArtEui(u1_t *buf) { /* Não utilizado neste projeto */ }
void os_getDevEui(u1_t *buf) { /* Não utilizado neste projeto */ }
void os_getDevKey(u1_t *buf) { /* Não utilizado neste projeto */ }

//*****************************************************************************************************************
// Initial settings
//*****************************************************************************************************************

void setup()
{

  Serial.begin(SERIAL_DEBUG);

  pinMode(pinToPushButton, INPUT);

  pixels1.begin();

  pixels1.clear();
  pixels1.setPixelColor(0, cores[3]);
  pixels1.show();
  delay(2000);
  pixels1.clear();
  pixels1.setPixelColor(0, cores[4]);
  pixels1.show();

  dht.begin();

  readDHT11();

  uint8_t appskey[sizeof(APPSKEY)];
  uint8_t nwkskey[sizeof(NWKSKEY)];
  int b;

  // Inicializa comunicação SPI com rádio LoRa
  SPI.begin(RADIO_SCLK_PORT, RADIO_MISO_PORT, RADIO_MOSI_PORT);

  // Inicializa stack LoRaWAN
  os_init();
  LMIC_reset();

  // Inicializa chaves usadas na comunicação ABP
  memcpy_P(appskey, APPSKEY, sizeof(APPSKEY));
  memcpy_P(nwkskey, NWKSKEY, sizeof(NWKSKEY));
  LMIC_setSession(0x13, DEVADDR, nwkskey, appskey);

  // Faz inicializações de rádio pertinentes a região do gateway LoRaWAN (ATC / Everynet Brasil)
  for (b = 0; b < 8; ++b)
    LMIC_disableSubBand(b);

  LMIC_enableChannel(0); // 915.2 MHz
  LMIC_enableChannel(1); // 915.4 MHz
  LMIC_enableChannel(2); // 915.6 MHz
  LMIC_enableChannel(3); // 915.8 MHz
  LMIC_enableChannel(4); // 916.0 MHz
  LMIC_enableChannel(5); // 916.2 MHz
  LMIC_enableChannel(6); // 916.4 MHz
  LMIC_enableChannel(7); // 916.6 MHz

  LMIC_setAdrMode(0);
  LMIC_setLinkCheckMode(0);

  // Data rate para janela de recepção RX2
  LMIC.dn2Dr = DR_SF12CR;

  // Configura data rate de transmissão e ganho do rádio LoRa (dBm) na transmissão
  LMIC_setDrTxpow(DR_SF12, GANHO_LORA_DBM);

  // Inicializa geração de número aleatório
  randomSeed(0);

  // Força primeiro envio de pacote LoRaWAN
  do_send(&sendjob);

} // end setup

//*****************************************************************************************************************
// Loop infinite
//*****************************************************************************************************************

void loop()
{

  // readDHT11();
  // delay(1000*timeToWait);

  os_runloop_once();

  if (!digitalRead(pinToPushButton))
    flagToPushButton = true;
  if (digitalRead(pinToPushButton) && flagToPushButton)
  {

    pixels1.clear();
    pixels1.setPixelColor(0, cores[0]);
    pixels1.show();

    flagToPushButton = false;
    os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(TX_INTERVAL), do_send);

    delay(1000 * timeToWait);
    pixels1.clear();
    pixels1.setPixelColor(0, cores[1]);
    pixels1.show();

  } // end if

} // end loop

//*****************************************************************************************************************
// Function to callback event
//*****************************************************************************************************************

void onEvent(ev_t ev)
{

#ifdef DEBUG
  Serial.print(os_getTime());
  Serial.print(": ");
  Serial.println(ev);
#endif

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
    Serial.println(millis());
    Serial.println(F("EV_TXCOMPLETE (incluindo espera pelas janelas de recepção)"));

    // Verifica se ack foi recebido do gateway
    if (LMIC.txrxFlags & TXRX_ACK)
      Serial.println(F("Ack recebido"));

    // Verifica se foram recebidos dados do gateway
    if (LMIC.dataLen)
    {

      Serial.println(F("Recebidos "));
      Serial.println(LMIC.dataLen);
      Serial.println(F(" bytes (payload) do gateway"));

      // Como houve recepção de dados do gateway, os coloca em um array para uso futuro.
      if (LMIC.dataLen == 1)
      {

        uint8_t dados_recebidos = LMIC.frame[LMIC.dataBeg + 0];
        Serial.print(F("Dados recebidos: "));
        Serial.write(dados_recebidos);

      } // end if

      // Aguarda 100ms para verificar novos eventos
      delay(100 * timeToWait);

    } // end if

    // Agenda próxima transmissão de dados ao gateway, informando daqui quanto tempo deve ocorrer (TX_INTERVAL) e qual função chamar para transmitir (do_send). Dessa forma, os eventos serão gerados de forma periódica.

    // os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
    break;

  case EV_LOST_TSYNC:
    Serial.println(F("EV_LOST_TSYNC"));
    break;
  case EV_RESET:
    Serial.println(F("EV_RESET"));
    break;
  case EV_RXCOMPLETE:
    Serial.println(F("EV_RXCOMPLETE"));
    break;
  case EV_LINK_DEAD:
    Serial.println(F("EV_LINK_DEAD"));
    break;
  case EV_LINK_ALIVE:
    Serial.println(F("EV_LINK_ALIVE"));
    break;
  case EV_TXSTART:
    Serial.println(F("EV_TXSTART"));
    Serial.println(millis());
    Serial.println(LMIC.freq);
    break;
  default:
    Serial.print(F("Evento desconhecido: "));
    Serial.println((unsigned)ev);
    break;

  } // end switch

} // end onEvent

//*****************************************************************************************************************
// Function to send LoRaWAN
//*****************************************************************************************************************

void do_send(osjob_t *j)
{

  pixels1.clear();
  pixels1.setPixelColor(0, cores[1]);
  pixels1.show();

  DynamicJsonDocument doc(MSG_BUFFER_SIZE); // Allocate the JSON document
  doc["t"] = String(valueTemperature);
  doc["h"] = String(valueHumididty);
  serializeJson(doc, msg);

  Serial.println(msg);

  static uint8_t mydata[tamanhoDados];
  memcpy(mydata, (uint8_t *)&msg, strlen(msg));

  /* Tentativa 02
    static uint8_t mydata[5];
    char mydata_str[5] 		= {0};
    int numero_aleatorio 	= (int)random(100);

    // Formata dado a ser enviado (número aleatório de 0 a 99)
    sprintf(mydata_str, "01%02d", numero_aleatorio);
    memcpy(mydata, (uint8_t *)&mydata_str, strlen(mydata_str));
  */

  // Verifica se já há um envio sendo feito. Em caso positivo, o envio atual é suspenso.
  if (LMIC.opmode & OP_TXRXPEND)
  {

#ifdef DEBUG
    Serial.println(F("OP_TXRXPEND: ha um envio ja pendente, portanto o atual envio nao sera feito"));
#endif

  } // end if

  else
  {
    // Aqui, o envio pode ser feito. O pacote LoRaWAN é montado e o coloca na fila de envio.
    LMIC_setTxData2(4, mydata, sizeof(mydata), 0);

#ifdef DEBUG
    Serial.println(F("Pacote LoRaWAN na fila de envio."));
    Serial.print(F("Enviados "));
    Serial.print(LMIC.dataLen);
    Serial.println(F(" bytes (payload) do end-device"));
#endif
    // Serial.println(F("RECONFIG."));
    // os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);

  } // end else

  delay(200);
  pixels1.clear();
  pixels1.setPixelColor(0, cores[4]);
  pixels1.show();

} // end do_send

//*****************************************************************************************************************
// Function to read dht11
//*****************************************************************************************************************

void readDHT11()
{

  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f))
  {

#ifdef DEBUG
    Serial.println(F("Failed to read from DHT sensor!"));
#endif

    return;

  } // end if

#ifdef DEBUG
  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("%"));
  Serial.print(F("  /  "));
  Serial.print(F("Temperature: "));
  Serial.print(t);
  Serial.println(F("°C "));
#endif

  valueTemperature = t;
  valueHumididty = h;

} // end readDHT11