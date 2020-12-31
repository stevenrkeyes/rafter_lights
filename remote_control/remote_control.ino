#include <SPI.h>
#include "RF24.h"

#define HEADER_BYTE 0xA5
#define NULL_CMD    0x00

/* Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 9 (enable) and 10 (chip select) */
RF24 radio(9,10);

// radio addresses; this device will be master #1 and will send to listener #1
byte listener_radio_address[] = "1list";
byte master_radio_address[] = "1mast";

void setup() {
  Serial.begin(115200);
  
  // radio setup
  radio.begin();

  // Set the PA Level low to prevent power supply related issues just in case,
  // and the likelihood of close proximity of the devices. RF24_PA_MAX is default.
  radio.setPALevel(RF24_PA_LOW);
  radio.openWritingPipe(listener_radio_address);

  radio.stopListening();
}

void loop() {
  byte data_byte;
  byte data[] = {0xA5, 0x00, 0x5A};
  
  Serial.println("loop start");

  if (!radio.write(data, sizeof(data))) {
    Serial.println("failed send");
  }
  
  Serial.println("sent data");

  while(1);
}

