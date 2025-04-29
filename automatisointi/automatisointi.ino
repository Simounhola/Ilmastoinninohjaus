
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#define DS75_ADDR 0x48
#include "settings.h"

const uint16_t kIrLedPin = 14;
IRsend irsend(kIrLedPin);

bool ensimmainenKierros = true;
String viimeinenAikaleima = "";
bool automaatioKaytossa = true;

unsigned long viimeinenAutomaattinenTarkistus = 0;
const unsigned long automaatioTarkistusvali = 30000;

unsigned long viimeinenSaatoAika = 0;
const unsigned long saatoViive = 3000;

struct IlmastointiTila {
  bool paalla;
  String moodi;
  int lampotila;
  String puhallus;
  bool oletustila;
};

IlmastointiTila tila = {false, "cool", 22, "low", true};


void paivitaTilaFirebaseen() {
  HTTPClient http;
  String url = String(DATABASE_URL) + "/tila.json?auth=" + DATABASE_SECRET;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  String payload = "{\"paalla\":" + String(tila.paalla ? "true" : "false") +
                   ",\"moodi\":\"" + tila.moodi +
                   "\",\"lampotila\":" + String(tila.lampotila) +
                   ",\"puhallus\":\"" + tila.puhallus + "\"}";
  http.PUT(payload);
  http.end();
  Serial.println(" Tila päivitetty Firebaseen.");
}

void paivitaAutomaattinenAsetus() {
  HTTPClient http;
  String url = String(DATABASE_URL) + "/tila/automaattinen.json?auth=" + DATABASE_SECRET;
  http.begin(url);
  int code = http.GET();
  
  if (code == 200) {
    String res = http.getString();
    res.trim();

    if (res == "true") {
      if (!automaatioKaytossa) {
        Serial.println(" Automaattinen ohjaus: PÄÄLLÄ");
      }
      automaatioKaytossa = true;
    } else if (res == "false") {
      if (automaatioKaytossa) {
        Serial.println(" Automaattinen ohjaus: POIS");
      }
      automaatioKaytossa = false;
    } else {
      Serial.println(" Tuntematon arvo automaatiossa: " + res + " – säilytetään nykyinen asetus");
    }
  } else {
    Serial.printf(" Automaattinen asetus GET epäonnistui (koodi %d) – säilytetään nykyinen asetus\n", code);
  }

  http.end();
}

void suoritaAsteittainenSaato() {
  static int vaihe = 0;
  unsigned long nyt = millis();

  if (!automaatioKaytossa || nyt - viimeinenSaatoAika < saatoViive || !tila.paalla) return;
  viimeinenSaatoAika = nyt;

  if (vaihe == 0 && tila.moodi != "cool") {
    tila.moodi = "cool";
    irsend.sendNEC(0x1FEB946, 32);
    Serial.println(" Asetetaan moodi: cool");
  } else if (vaihe == 1 && tila.lampotila != 20) {
    if (tila.lampotila < 20) {
      tila.lampotila++;
      irsend.sendNEC(0x1FE29D6, 32);
      Serial.println(" Nostetaan lämpötilaa");
    } else {
      tila.lampotila--;
      irsend.sendNEC(0x1FEA956, 32);
      Serial.println(" Lasketaan lämpötilaa");
    }
  } else if (vaihe == 2 && tila.puhallus != "low") {
    tila.puhallus = "low";
    irsend.sendNEC(0x1FE6996, 32);
    Serial.println(" Asetetaan puhallus: low");
  }

  vaihe = (vaihe + 1) % 3;
  paivitaTilaFirebaseen();
}

void lahetaLampotilaFirebaseen() {
  Wire.beginTransmission(DS75_ADDR);
  Wire.write(0x00);
  Wire.endTransmission(false);
  Wire.requestFrom(DS75_ADDR, 2);

  if (Wire.available() == 2) {
    uint8_t msb = Wire.read();
    uint8_t lsb = Wire.read();
    float temperature = ((msb << 8) | lsb) / 256.0;

    Serial.printf(" %.2f °C\n", temperature);

    HTTPClient http;
    String url = String(DATABASE_URL) + "/sensorit/lampotila.json?auth=" + DATABASE_SECRET;
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    http.PUT(String(temperature, 2));
    http.end();

    if (automaatioKaytossa) {
      if (temperature > 27.0 && !tila.paalla) {
        tila.paalla = true;
        irsend.sendNEC(0x1FE39C6, 32);
        Serial.println(" Käynnistetään ilmastointi");
        paivitaTilaFirebaseen();
        delay(2000);
      }

      if (tila.paalla && temperature > 27.0) {
        suoritaAsteittainenSaato();
      }

      if (tila.paalla && temperature < 22.0) {
        Serial.println(" Sammutetaan ilmastointi (liian kylmä)");
        tila.paalla = false;
        irsend.sendNEC(0x1FE39C6, 32);
        paivitaTilaFirebaseen();
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  irsend.begin();
  Wire.begin();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print(" Yhdistetään WiFiin");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n WiFi yhdistetty.");

  HTTPClient http;
  http.begin(String(DATABASE_URL) + "/tila.json?auth=" + DATABASE_SECRET);
  int code = http.GET();
  if (code == 200) {
    String res = http.getString();
    tila.paalla = res.indexOf("\"paalla\":true") != -1;
    tila.moodi = res.indexOf("dry") != -1 ? "dry" : (res.indexOf("fan") != -1 ? "fan" : "cool");
    tila.puhallus = res.indexOf("high") != -1 ? "high" : "low";
    tila.lampotila = 22;
    tila.oletustila = false;
  }
  http.end();

  paivitaTilaFirebaseen();
  paivitaAutomaattinenAsetus();
}

void loop() {
  lahetaLampotilaFirebaseen();

  unsigned long nyt = millis();
  if (nyt - viimeinenAutomaattinenTarkistus > automaatioTarkistusvali) {
    paivitaAutomaattinenAsetus();
    viimeinenAutomaattinenTarkistus = nyt;
  }

  HTTPClient http;
  http.begin(String(DATABASE_URL) + "/komento.json?auth=" + DATABASE_SECRET);
  int code = http.GET();

  if (code == 200) {
    String vastaus = http.getString();
    String komento = "", aikaleima = "";

    int vPos = vastaus.indexOf("\"viesti\"");
    if (vPos != -1) {
      int start = vastaus.indexOf("\"", vPos + 8) + 1;
      int end = vastaus.indexOf("\"", start);
      komento = vastaus.substring(start, end);
    }

    int tPos = vastaus.indexOf("\"timestamp\"");
    if (tPos != -1) {
      int start = vastaus.indexOf("\"", tPos + 11) + 1;
      int end = vastaus.indexOf("\"", start);
      aikaleima = vastaus.substring(start, end);
    }

    if (ensimmainenKierros) {
      viimeinenAikaleima = aikaleima;
      ensimmainenKierros = false;
      http.end();
      return;
    }

    if (aikaleima != viimeinenAikaleima && komento.length() > 0) {
      viimeinenAikaleima = aikaleima;
      Serial.println(" Komento: " + komento);

      if (komento == "virtakytkin") {
        tila.paalla = !tila.paalla;
        irsend.sendNEC(0x1FE39C6, 32);
      } else if (komento == "lämpötila_plus" && tila.moodi == "cool" && tila.lampotila < 30) {
        tila.lampotila++;
        irsend.sendNEC(0x1FE29D6, 32);
      } else if (komento == "lämpötila_miinus" && tila.moodi == "cool" && tila.lampotila > 16) {
        tila.lampotila--;
        irsend.sendNEC(0x1FEA956, 32);
      } else if (komento == "puhallus") {
        tila.puhallus = tila.puhallus == "low" ? "high" : "low";
        irsend.sendNEC(0x1FE6996, 32);
      } else if (komento == "tila") {
        if (tila.moodi == "cool") tila.moodi = "dry";
        else if (tila.moodi == "dry") {
          tila.moodi = "fan";
          tila.puhallus = "low";
        } else tila.moodi = "cool";
        irsend.sendNEC(0x1FEB946, 32);
      }

      paivitaTilaFirebaseen();
    }
  }

  http.end();
  delay(1000);
}
