// Importe les bibliothèques nécessaires.

#include <Adafruit_NeoPixel.h>

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include "LittleFS.h"

// Définit la configuration générale

#define WIFI_SSID "Nom_Wifi" // Définis le nom du réseau Wi-Fi
#define WIFI_PASSWORD "MotDePasse_Wifi" // Définis le mot de passe du réseau Wi-Fi
#define WIFI_MAXTRY 15 // Définis le nombre de tentative de connexion au réseau Wi-Fi
#define SERVER_PORT 80 // Définis le port utilisé par le serveur HTTP
#define HOST_NAME "nanoleaf-diy" // Définis le nom d'hôte du serveur propagé par mDNS
#define OTA_NAME "OTA NanoLeaf DIY" // Définis le nom de la connexion OTA (mise à jour sans fil)
#define OTA_PASSWORD "diy" // Définis le mot de passe de la connexion OTA (mise à jour sans fil)

#define LED_PIN D2 // Définis la broche sur laquelle est branchée le câble DATA du strip LED (en utilisant la broche D4, la LED intégrée à la carte sera également constamment allumée)
#define IS_RGBW 1 // Définis si le strip LED est de type RGBW (Oui : 1 ; Non : 0)
#define LED_COUNT 12 // Définis le nombre de LED par panneau NanoLeaf
#define PANEL_COUNT 5 // Définis le nombre de panneaux NanoLeaf
#define PANEL_DIRECTION "horizontal" // Définis la direction des panneaux NanoLeaf (Montage horizontal : "horizontal" ; Montage vertical : "vertical")
#define PANEL_GRID "1-2-top,2-2-bottom,2-1-top,3-1-bottom,4-1-top" // Définis la structure des panneaux NanoLeaf

/*
 * 
 * | 1-1 | 2-1 | 3-1 | 4-1 | 5-1 | ... |
 * | 1-2 | 2-2 | 3-2 | 4-2 | 5-2 | ... |
 * | 1-3 | 2-3 | 3-3 | 4-3 | 5-3 | ... |
 * | ... | ... | ... | ... | ... | ... |
 * 
 */
 
#define POWER 1 // Définis l'état de l'alimentation par défaut (Allumé : 1 ; Éteint : 0)
#define BRIGHTNESS 50 // Définis la luminosité par défaut (Min : 0 ; Max : 255)
#define COLOR_R 255 // Définis la valeur du canal rouge de la couleur par défaut (Min : 0 ; Max : 255)
#define COLOR_G 255 // Définis la valeur du canal vert de la couleur par défaut (Min : 0 ; Max : 255)
#define COLOR_B 0 // Définis la valeur du canal bleu de la couleur par défaut (Min : 0 ; Max : 255)

// Initialise les variables

Adafruit_NeoPixel strip(LED_COUNT * PANEL_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800); // Crée l'objet strip (nombre de LED, pin DATA, type de strip LED)
ESP8266WebServer server(SERVER_PORT); // Crée l'objet server (port du serveur)

uint8_t power = POWER;
uint8_t brightness = BRIGHTNESS;
uint32_t color = strip.Color(COLOR_R, COLOR_G, COLOR_B);
uint32_t colors[PANEL_COUNT];

// Script

void setup() {
  Serial.begin(115200);
  Serial.println("Démarrage du moniteur série");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Connexion Wi-Fi en cours");

  uint8_t count_try = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    count_try++;
    if (count_try >= WIFI_MAXTRY) {
      Serial.printf("\nImpossible de se connecter au réseau Wi-Fi %s après %i tentatives\n", WIFI_SSID, WIFI_MAXTRY);
      ESP.deepSleep(5*60*1000000, WAKE_RF_DEFAULT); // Met l'ESP8266 en sommeil profond pendant 5 minutes
    }
  }

  IPAddress ip = WiFi.localIP();
  Serial.printf("\nConnecté au réseau Wi-Fi %s avec l'adresse IP %d.%d.%d.%d\n", WIFI_SSID, ip[0], ip[1], ip[2], ip[3]);

  ArduinoOTA.setPort(8266);
  ArduinoOTA.setHostname(OTA_NAME);
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.begin();
  Serial.printf("Démarrage de la mise à jour sans fil (OTA) %s à l'adresse %i.%i.%i.%i:%i\n", OTA_NAME, ip[0], ip[1], ip[2], ip[3], 8266);

  LittleFS.begin();
  Serial.println("Montage du disque flash de l'ESP8266");

  server.on("/config", HTTP_GET, []() {
    String json = "{\"power\":" + String(power) + ", \"brightness\":" + String(brightness) + ", \"color\":\"" + colorToRGB(color) + "\", \"panels\":" + String(PANEL_COUNT) + ", \"grid\":\"" + PANEL_GRID + "\", \"direction\": \"" + PANEL_DIRECTION + "\"}";

    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json", json);
  });

  server.on("/colors", HTTP_GET, []() {
    String json = "{\"panels\":"+ String(PANEL_COUNT) + ", \"colors\": [";
    for(uint8_t i = 0; i < PANEL_COUNT; i++) {
      json += "\"" + colorToRGB(colors[i]) + "\"";
      if(i < (PANEL_COUNT - 1)) {
        json += ",";
      }
    }
    json += "]}";

    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json", json);
  });

  server.on("/power", HTTP_POST, []() {
    String value = server.arg("value");

    if(value.toInt() == 0) {
      strip.clear();
      strip.show();
      power = 0;
    }
    if(value.toInt() == 1) {
       for(uint8_t i = 0; i < PANEL_COUNT; i++) {
         uint8_t first, count;
         first = i * LED_COUNT;
         count = LED_COUNT;

         strip.fill(colors[i], first, count);
         strip.show();
      }
      power = 1;
    }

    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/plain", String(value));
  });

  server.on("/brightness", HTTP_POST, []() {
    String value = server.arg("value");
    brightness = value.toInt();
    if(brightness == 0) {
      power = 0;
    }

    strip.setBrightness(brightness);
    strip.show();

    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/plain", String(value));
  });

  server.on("/color", HTTP_POST, []() {
    String red = server.arg("red");
    String green = server.arg("green");
    String blue = server.arg("blue");
    String panels = server.arg("panels");

    if(IS_RGBW && red.toInt() == 255 && green.toInt() == 255 && blue.toInt() == 255) {
      color = strip.Color(0, 0, 0, 255);
    } else {
      color = strip.Color(red.toInt(), green.toInt(), blue.toInt());
    }

    if(power == 1) {
      uint8_t index, panel;
      String delimiter = ",";

      while(panels != "") {
        index = panels.indexOf(delimiter);
        panel = panels.substring(0, index).toInt();
        panels.remove(0, index + delimiter.length());

        uint8_t first, count;
        first = (panel - 1) * LED_COUNT;
        count = LED_COUNT;

        strip.fill(color, first, count);
        strip.show();
      }

      for(uint8_t i = 0; i < PANEL_COUNT; i++) {
        colors[i] = strip.getPixelColor(i * LED_COUNT);
      }
    }

    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/plain", String(red) + "," + String(green) + "," + String(blue));
  });

  server.serveStatic("/js", LittleFS, "/js");
  server.serveStatic("/css", LittleFS, "/css");
  server.serveStatic("/img", LittleFS, "/img");
  server.serveStatic("/", LittleFS, "/index.html");

  server.onNotFound(handleNotFound);

  MDNS.begin(HOST_NAME);
  MDNS.setHostname(HOST_NAME);

  server.begin();
  Serial.printf("Démarrage du serveur HTTP à l'adresse http://%s.local ou %i.%i.%i.%i:%i\n", HOST_NAME, ip[0], ip[1], ip[2], ip[3], SERVER_PORT);

  for(uint8_t i = 0; i < PANEL_COUNT; i++) {
    colors[i] = color;
  }

  strip.begin();
  if(POWER == 1) {
    color = strip.Color(COLOR_R, COLOR_G, COLOR_B);
    strip.fill(color);
  }
  strip.show();
  strip.setBrightness(BRIGHTNESS);
  Serial.println("Démarrage de la bande LED");
}

void loop() {
  ArduinoOTA.handle();
  server.handleClient();
  MDNS.update();
}

void handleNotFound() {
  String message = "Fichier introuvable\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

String colorToRGB(uint32_t color) {
  if(IS_RGBW && color == strip.Color(0, 0, 0, 255)) {
    return "255,255,255";
  } else {
    uint8_t red = (color>>16) & 255;
    uint8_t green = (color>>8) & 255;
    uint8_t blue = color & 255;
    return String(red) + "," + String(green) + "," + String(blue);
  }
}
