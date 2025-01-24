/*
dejan.rakijasic@gmail.com

Primjer: komunikacija CAN protokolom

CAN libraray: 
mcp_can by coryjfowler 1.5.1
MCP_CAN Library. Adds support for Microchip CAN 2.0B protocol controllers (MCP2515, MCP25625, and similar)

- povezivanje CAN bus modula - MCP2515
      VCC - 5V
      GND - GND
      CS - pin 10
      SO - pin 12
      SI - pin 11
      SCK - pin 13
      INT - pin 2

MASKIRANJE I FILTRIRANJE PORUKA:

- maska (init_Mask) definira koji bitovi ID-a poruke se provjeravaju (1 - provjeravaju, 0 - ignoriraju)
- filtri (init_Filt) definiraju koje vrijednosti tih bitova se smatraju "usklađenima", tj. propuštaju se.

  CAN0.init_Mask(mask_number, ext_flag, mask_value);

1. argument: mask_number - označava koju masku postavljamo.
    MCP2515 podržava dvije 'maske':
        Mask 0 
        Mask 1 

2. argument: ext_flag - označava hoće li se maska koristiti za standardni (11-bitni) ili prošireni (29-bitni) ID.
        0: Standardni ID (11-bitni).
        1: Prošireni ID (29-bitni).

3. argument: mask_value - vrijednost maske, koja definira koje bitove ID-a poruke treba provjeravati. 


  CAN0.init_Filt(filt_number, ext_flag, filt_value)

1. argument: filt_number (broj filtra):
        MCP2515 podržava do 6 filtera (0-5) i svaki je povezan s određenom maskom (Mask 0 ili Mask 1)

2. argument: ext_flag (Prošireni ID):
        0: Standardni ID (11-bitni).
        1: Prošireni ID (29-bitni).

3. argument: filt_value (Vrijednost filtra): specifične ID vrijednosti koje trebaju proći kada se usporede s relevantnim dijelovima maske.


  - PRIMJER FILTRA i MASKI za standardni ID (11-bitova):

                          CAN0.init_Mask(0, 0, 0x010F0000); // Maska 0
                          CAN0.init_Filt(0, 0, 0x01000000); // Filtar 0
                          CAN0.init_Filt(1, 0, 0x01010000); // Filtar 1

                          CAN0.init_Mask(1, 0, 0x010F0000); // Maska 1
                          CAN0.init_Filt(2, 0, 0x01030000); // Filtar 2
                          CAN0.init_Filt(3, 0, 0x01040000); // Filtar 3
                          CAN0.init_Filt(4, 0, 0x01060000); // Filtar 4
                          CAN0.init_Filt(5, 0, 0x01070000); // Filtar 5

                             MASKA: 0000 0001 0000 1111 0000 0000 0000 0000 (pokriva specifične bitove '1' za 11-bitni CAN ID)

                          ID (hex)	maskirani ID (binarno)	Prolazi?	Filtar
                          0x100	    0000 0001 0000 0000	      Da	    Filtar 0
                          0x101	    0000 0001 0000 0001     	Da	    Filtar 1
                          0x102	    0000 0001 0000 0010	      Ne	    Nema podudaranja
                          0x103	    0000 0001 0000 0011	      Da	    Filtar 2
                          0x104	    0000 0001 0000 0100	      Da	    Filtar 3
                          0x105	    0000 0001 0000 0101	      Ne	    Nema podudaranja
                          0x106	    0000 0001 0000 0110	      Da	    Filtar 4
                          0x107	    0000 0001 0000 0111	      Da	    Filtar 5
*/


#include <mcp_can.h>
#include <SPI.h>

#define CAN0_INT 2        // Interrupt pin za CAN modul
#define CAN_CS_PIN 10     // Chip select pin

#define GPIO_PIN 7 //LED na Arduino Uno pin7

MCP_CAN CAN0(CAN_CS_PIN); // Definiraj CAN0 pin

// Definiranje ID uređaja (standardni ID 11-bitni)
long unsigned int myCAN_ID = 0x100;

byte data[8] = {0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8};  // Podaci koji se šalju

long unsigned int rxId;
unsigned char len = 0;
unsigned char rxBuf[8];
char msgString[128];

void setup() {
  Serial.begin(115200);  
  pinMode(GPIO_PIN, OUTPUT);

  // Inicijalizacija MCP2515 CAN modula
  if (CAN0.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK) {
    Serial.println("MCP2515 Initialized Successfully!");
  } else {
    Serial.println("Error Initializing MCP2515...");
    while (1);  
  }

  CAN0.setMode(MCP_NORMAL);  // Postavljanje načina rada na NORMAL
  pinMode(CAN0_INT, INPUT);  // Postavljanje pina za INT input

  Serial.println("MCP2515 CAN send/receive example...");
}

void loop() {
  // Primanje poruka
  if (!digitalRead(CAN0_INT)) {  // Ako je CAN0_INT pin 'LOW', pročitaj primljenu poruku
    CAN0.readMsgBuf(&rxId, &len, rxBuf);  // Pročitaj podatke: rxId = ID, len = duljina podataka, rxBuf = podaci

    if ((rxId & 0x80000000) == 0x80000000) {  // Provjera je li ID CAN2.0B (29 bitova) ili CAN2.0A (11 bitova)
      sprintf(msgString, "Extended ID: 0x%.8lX  DLC: %1d  Data:", (rxId & 0x1FFFFFFF), len);
    } else {
      sprintf(msgString, "Standard ID: 0x%.3lX       DLC: %1d  Data:", rxId, len);
    }

    Serial.print(msgString);

    if ((rxId & 0x40000000) == 0x40000000) {  // Provjera je li to Remote Request Frame
      sprintf(msgString, " REMOTE REQUEST FRAME");
      Serial.print(msgString);
    } else {
      for (byte i = 0; i < len; i++) { 
        sprintf(msgString, " 0x%.2X", rxBuf[i]);
        Serial.print(msgString);
      }
      
    }

    Serial.println();
    if (len > 0) {
    if (rxBuf[0] == 0x01) {
        digitalWrite(GPIO_PIN, HIGH);  
        Serial.println("GPIO_PIN uključen");
    } else if (rxBuf[0] == 0x00) {
        digitalWrite(GPIO_PIN, LOW);   
        Serial.println("GPIO_PIN isključen");
    } else {
        Serial.println("Nepoznata naredba");
    }
}

  }


  // Slanje poruke
  byte sndStat = CAN0.sendMsgBuf(myCAN_ID, 0, 8, data);  // Slanje poruke s ID-jem myCAN_ID
  if (sndStat == CAN_OK) {
    Serial.println("Message Sent Successfully!");
  } else {
    Serial.println("Error Sending Message...");
  }

  delay(10); 
}
