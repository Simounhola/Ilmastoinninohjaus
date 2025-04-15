#include <IRremoteESP8266.h>
#include <IRsend.h>

// IR-LEDin GPIO-pinni
const uint16_t irLedPin = 14;
IRsend irsend(irLedPin);

// NEC-protokollan IR-koodit
const uint32_t POWER_TOGGLE = 0x1FE39C6;
const uint32_t TEMP_UP      = 0x1FE29D6;
const uint32_t TEMP_DOWN    = 0x1FEA956;
const uint32_t FAN_SPEED    = 0x1FE6996;
const uint32_t MODE_CHANGE  = 0x1FEB946;

void tulostaValikko() {
  Serial.println("Valitse komento:");
  Serial.println("1 = Virtakytkin");
  Serial.println("2 = Lämpötila +");
  Serial.println("3 = Lämpötila -");
  Serial.println("4 = Puhallusnopeus");
  Serial.println("5 = Tilan vaihto");
}

void setup() {
  Serial.begin(115200); 
  irsend.begin();
  Serial.println("IR-ohjain käynnistetty");
  tulostaValikko();
}

void loop() {
  if (Serial.available()) {
    char valinta = Serial.read();

    switch (valinta) {
      case '1':
        Serial.println("Virtakytkin");
        irsend.sendNEC(POWER_TOGGLE, 32);
        break;
      case '2':
        Serial.println("Lämpötila +");
        irsend.sendNEC(TEMP_UP, 32);
        break;
      case '3':
        Serial.println("Lämpötila -");
        irsend.sendNEC(TEMP_DOWN, 32);
        break;
      case '4':
        Serial.println("Puhallusnopeus");
        irsend.sendNEC(FAN_SPEED, 32);
        break;
      case '5':
        Serial.println("Tilan vaihto");
        irsend.sendNEC(MODE_CHANGE, 32);
        break;
      default:
        Serial.println("Virheellinen valinta (1–5)");
        break;
    }

    while (Serial.available()) Serial.read(); 
    delay(500); 
    tulostaValikko();
  }
}
