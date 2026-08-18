#include <U8x8lib.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <WiFiS3.h> 

#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>
#include <Adafruit_SGP40.h>
#include <Adafruit_SGP30.h>
#include "SparkFun_SCD30_Arduino_Library.h"
#include <Seeed_HM330X.h>
#include <Multichannel_Gas_GMXXX.h>
#include <DHT20.h>
#include "arduino_secrets.h"

// --- Configuration Réseau Wi-Fi ---
const char ssid[] = SECRET_SSID;
const char pass[] = SECRET_PASS;
const char serverAddress[] = SERVER_ADDRESS;
const char ingestToken[] = EASE_INGEST_TOKEN;
const int serverPort = 5000;
WiFiClient client;

// --- Modules de base ---
U8X8_SSD1306_128X64_NONAME_HW_I2C oled(/* reset=*/ U8X8_PIN_NONE);
const int chipSelect = 4;
const char nomFichier[] = "all_log.csv"; // Nom court car le système FAT (SD) limite la taille des noms

// --- Objets Capteurs ---
Adafruit_BME680 bme;
Adafruit_SGP40 sgp40;
Adafruit_SGP30 sgp30;
SCD30 scd30;
HM330X hm3301;
GAS_GMXXX<TwoWire> gas;
DHT20 dht20;

// MH-Z16 sur Serial1
#define mhz16 Serial1
const unsigned char cmd_get_sensor[] = {0xff, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};

// --- Fonction Multiplexeur TCA9548A ---
#define TCAADDR 0x70
void tcaselect(uint8_t i) {
    if (i > 7) return;
    Wire.beginTransmission(TCAADDR);
    Wire.write(1 << i);
    Wire.endTransmission();
}

// --- Variables Globales pour stocker les mesures ---
const int BUTTON_PIN = 5;
bool screenOn = true;

struct CustomDateTime {
    int year, month, day, hour, minute, second;
};
CustomDateTime lireHeureRTC();
void forcerHeureRTC(int year, int month, int day, int hour, int minute, int second);

CustomDateTime current_dt;

// BME680
float bme_temp = 0.0, bme_hum = 0.0, bme_pres = 0.0, bme_voc = 0.0;
// SCD30
int scd_co2 = 0;
float scd_temp = 0.0, scd_hum = 0.0;
// MH-Z16
int mhz16_co2 = 0;
// HM3301
uint8_t hm3301_buf[30];
uint16_t pm1_0 = 0, pm2_5 = 0, pm10 = 0;
// SGP40
uint16_t sgp40_voc = 0;
// SGP30
uint16_t sgp30_tvoc = 0;
uint16_t sgp30_eco2 = 0;
// HCHO (Analogique)
float hcho_ppm = 0.0;
// À déterminer par étalonnage pour chaque WSP2110. La valeur ci-dessous
// est seulement une valeur initiale et ne permet pas une mesure certifiée.
const float HCHO_R0_KOHM = 34.28;
// DHT20
float dht20_temp = 0.0, dht20_hum = 0.0;
// Multichannel Gas
float valNO2 = 0.0, valEtOH = 0.0, valVOC = 0.0, valCO = 0.0;

// --- Gestion temporelle ---
unsigned long previousMillisData = 0;
const long intervalData = 10000; // 10 secondes entre chaque envoi de trame

unsigned long previousMillisSGP40 = 0;
const long intervalSGP40 = 1000; // 1 seconde (Obligatoire pour l'algorithme du SGP40)

unsigned long previousMillisOled = 0;
const long intervalOled = 4000;  // 4 secondes pour changer de page OLED
int oledPage = 0; 
unsigned long lastWifiReconnectAttempt = 0;

// ==========================================
// =               SETUP                    =
// ==========================================
void setup() {
    pinMode(BUTTON_PIN, INPUT);
    Serial.begin(115200); 
    mhz16.begin(9600); // Pour le MH-Z16
    Wire.begin(); 
    
    // Initialisation Écran OLED
    oled.begin();
    oled.setPowerSave(0); 
    oled.setFont(u8x8_font_chroma48medium8_r); 
    oled.clear();
    oled.setCursor(0, 0); oled.print("Init Start...");
    delay(1000);

    // Initialisation RTC
    Wire.beginTransmission(0x51);
    if (Wire.endTransmission() != 0) {
        Serial.println("Erreur RTC !");
        oled.setCursor(0, 2); oled.print("Erreur RTC !");
        while (1) delay(10);
    }
    // LIGNE À DÉCOMMENTER POUR METTRE À L'HEURE (puis re-commenter) :
    // forcerHeureRTC(2026, 6, 25, 20, 0, 0);

    // Connexion Wi-Fi
    oled.clear(); oled.setCursor(0, 0); oled.print("WiFi Connexion.");
    WiFi.begin(ssid, pass);
    int wifiTimeout = 0;
    while (WiFi.status() != WL_CONNECTED && wifiTimeout < 10) {
        delay(1000);
        Serial.print(".");
        wifiTimeout++;
    }
    if (WiFi.status() == WL_CONNECTED) {
        oled.setCursor(0, 2); oled.print("WiFi OK!");
    } else {
        oled.setCursor(0, 2); oled.print("WiFi Echoue :(");
    }
    delay(1500);

    // Initialisation des capteurs (I2C)
    oled.clear(); oled.setCursor(0, 0); oled.print("Init Capteurs");

    // 1. BME680 (canal 4 du multiplexeur)
    tcaselect(4);
    if (!bme.begin(0x76) && !bme.begin(0x77)) {
        Serial.println("Erreur BME680");
        oled.setCursor(0, 1); oled.print("BME Error");
    } else {
        bme.setTemperatureOversampling(BME680_OS_8X);
        bme.setHumidityOversampling(BME680_OS_2X);
        bme.setPressureOversampling(BME680_OS_4X);
        bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
        bme.setGasHeater(320, 150);
        oled.setCursor(0, 1); oled.print("BME OK");
    }

    // 2. SCD30 (Sur Hub Port 5)
    tcaselect(5);
    if (!scd30.begin()) {
        Serial.println("Erreur SCD30");
        oled.setCursor(0, 2); oled.print("SCD Error");
    } else {
        oled.setCursor(0, 2); oled.print("SCD OK");
    }

    // 3. HM3301 (Sur Hub Port 7)
    tcaselect(7);
    if (hm3301.init()) {
        Serial.println("Erreur HM3301");
        oled.setCursor(0, 3); oled.print("HM3 Error");
    } else {
        oled.setCursor(0, 3); oled.print("HM3 OK");
    }

    // 4. Multichannel Gas (Sur Hub Port 6)
    tcaselect(6);
    gas.begin(Wire, 0x08);
    oled.setCursor(0, 4); oled.print("Multi OK");

    // 5. SGP40 (Sur Hub Port 4)
    tcaselect(4);
    if (!sgp40.begin()) {
        Serial.println("Erreur SGP40");
        oled.setCursor(9, 4); oled.print("SGP Err");
    } else {
        oled.setCursor(9, 4); oled.print("SGP OK");
    }
    
    // 6. DHT20 (Sur Hub Port 3)
    tcaselect(3);
    dht20.begin();
    oled.setCursor(0, 5); oled.print("DHT20 OK");

    // 7. SGP30 (Sur Hub Port 2)
    tcaselect(2);
    if (!sgp30.begin()) {
        Serial.println("Erreur SGP30");
        oled.setCursor(9, 5); oled.print("S30 Err");
    } else {
        sgp30.IAQinit(); // INDISPENSABLE POUR DEMARRER LES MESURES
        oled.setCursor(9, 5); oled.print("S30 OK");
    }

    // Le capteur HCHO (WSP2110) est analogique et se branche sur le port A0 du Grove Base Shield.
    // Pas besoin d'initialisation I2C.
    
    // Initialisation SDCarte SD
    oled.clear(); oled.setCursor(0, 0);
    if (!SD.begin(chipSelect)) {
        Serial.println("Erreur SD !");
        oled.print("Erreur SD !");
        while (1) delay(10); 
    }
    oled.print("SD OK");
    
    // Création de l'en-tête CSV sur la carte SD
    File dataFile = SD.open(nomFichier, FILE_WRITE);
    if (dataFile) {
        if(dataFile.size() == 0) {
            dataFile.println("Date,Heure,Temp_BME(C),Hum_BME(%),Pression(hPa),VOC_BME(kOhms),CO2_SCD30(ppm),Temp_SCD30(C),Hum_SCD30(%),PM1.0,PM2.5,PM10,CO2_MHZ16(ppm),NO2,EtOH,VOC_Multi,CO,VOC_SGP40,Temp_DHT20(C),Hum_DHT20(%),TVOC_SGP30(ppb),eCO2_SGP30(ppm),HCHO(ppm)");
        }
        dataFile.close();
    }
    
    delay(1000);
    effectuerLectureEtAffichage(); // Première lecture immédiate
}

// ==========================================
// =                 LOOP                   =
// ==========================================
void loop() {
    unsigned long currentMillis = millis();
    
    // Handle OLED toggle button
    static bool lastButtonState = LOW;
    bool currentButtonState = digitalRead(BUTTON_PIN);
    if (currentButtonState == HIGH && lastButtonState == LOW) {
        screenOn = !screenOn;
        oled.setPowerSave(screenOn ? 0 : 1);
        delay(50); // debounce
    }
    lastButtonState = currentButtonState;
    
    if (currentMillis - previousMillisOled >= intervalOled) {
        previousMillisOled = currentMillis;
        oledPage = (oledPage + 1) % 6; 
        afficherOLED();
    }
    
    // SGP40 doit absolument être lu toutes les secondes (1Hz) pour son algorithme
    if (currentMillis - previousMillisSGP40 >= intervalSGP40) {
        previousMillisSGP40 = currentMillis;
        tcaselect(4);
        sgp40_voc = sgp40.measureVocIndex(bme_temp, bme_hum);
    }
    
    // Traitement des données et envoi Wi-Fi / Écriture SD
    if (currentMillis - previousMillisData >= intervalData) {
        previousMillisData = currentMillis;
        effectuerLectureEtAffichage();
    }
}


// ==========================================
// =              FONCTIONS                 =
// ==========================================
void effectuerLectureEtAffichage() {
    current_dt = lireHeureRTC();
    
    // Lecture BME680
    tcaselect(4);
    if (bme.performReading()) {
        bme_temp = bme.temperature;
        bme_hum = bme.humidity;
        bme_pres = bme.pressure / 100.0;
        bme_voc = bme.gas_resistance / 1000.0;
    }
    
    // Lecture SCD30 (Port 5)
    tcaselect(5);
    if (scd30.dataAvailable()) {
        scd_co2 = scd30.getCO2();
        scd_temp = scd30.getTemperature();
        scd_hum = scd30.getHumidity();
    }
    
    // Lecture MH-Z16 (UART)
    readMHZ16();
    
    // Lecture HM3301 (Particules) (Port 7)
    tcaselect(7);
    if (!hm3301.read_sensor_value(hm3301_buf, 29)) {
        pm1_0 = (uint16_t)hm3301_buf[4] << 8 | hm3301_buf[5];
        pm2_5 = (uint16_t)hm3301_buf[6] << 8 | hm3301_buf[7];
        pm10  = (uint16_t)hm3301_buf[8] << 8 | hm3301_buf[9];
    }
    
    // Lecture Multichannel Gas (Port 6)
    tcaselect(6);
    valNO2  = gas.measure_NO2();
    valEtOH = gas.measure_C2H5OH();
    valVOC  = gas.measure_VOC();
    valCO   = gas.measure_CO();
    
    // Lecture SGP40 (Port 4)
    tcaselect(4);
    // Utilisation de la temp et hum du BME680    // La lecture du SGP40 a été déplacée dans le loop à 1Hz
    // car son algorithme a besoin d'une lecture par seconde pour fonctionner.
    
    // Lecture DHT20 (Port 3)
    tcaselect(3);
    dht20.read();
    dht20_temp = dht20.getTemperature();
    dht20_hum = dht20.getHumidity();
    
    // Lecture SGP30 (Port 2)
    tcaselect(2);
    if (sgp30.IAQmeasure()) {
        sgp30_tvoc = sgp30.TVOC;
        sgp30_eco2 = sgp30.eCO2;
    }
    
    // Lecture HCHO (Analogique sur port A0)
    int sensorValue = analogRead(A0);
    if (sensorValue <= 0) sensorValue = 1;
    if (sensorValue > 1022) sensorValue = 1022;
    float Rs = (1023.0 / sensorValue) - 1.0;
    hcho_ppm = pow(10.0, ((log10(Rs / HCHO_R0_KOHM) - 0.0827) / (-0.4807)));
    
    // Formatage CSV
    char dateStr[11], heureStr[9];
    sprintf(dateStr, "%02d/%02d/%04d", current_dt.day, current_dt.month, current_dt.year);
    sprintf(heureStr, "%02d:%02d:%02d", current_dt.hour, current_dt.minute, current_dt.second);
    
    String csvLine = String(dateStr) + "," + String(heureStr) + "," + 
                     String(bme_temp, 2) + "," + String(bme_hum, 2) + "," + String(bme_pres, 2) + "," + String(bme_voc, 2) + "," +
                     String(scd_co2) + "," + String(scd_temp, 2) + "," + String(scd_hum, 2) + "," +
                     String(pm1_0) + "," + String(pm2_5) + "," + String(pm10) + "," +
                     String(mhz16_co2) + "," + String(valNO2, 2) + "," + String(valEtOH, 2) + "," + String(valVOC, 2) + "," + String(valCO, 2) + "," + String(sgp40_voc) + "," +
                     String(dht20_temp, 2) + "," + String(dht20_hum, 2) + "," +
                     String(sgp30_tvoc) + "," + String(sgp30_eco2) + "," + String(hcho_ppm, 2);
    
    // Sauvegarde sur SD
    File dataFile = SD.open(nomFichier, FILE_WRITE);
    if (dataFile) {
        dataFile.println(csvLine);
        dataFile.close();
        Serial.println("SD OK: " + csvLine);
    }
    
    // Envoi Wi-Fi
    ensureWiFiConnected();
    if (WiFi.status() == WL_CONNECTED) {
        if (client.connect(serverAddress, serverPort)) {
            client.println("POST /data HTTP/1.1");
            client.println("Host: " + String(serverAddress));
            client.println("Content-Type: text/plain");
            if (strlen(ingestToken) > 0) {
                client.println("X-EASE-Token: " + String(ingestToken));
            }
            client.print("Content-Length: ");
            client.println(csvLine.length());
            client.println();
            client.print(csvLine);
            client.stop();
        } else {
            Serial.println("Erreur: Serveur Python inaccessible.");
        }
    }
    
    // Force la mise à jour immédiate de l'écran avec les nouvelles données
    afficherOLED();
}

void ensureWiFiConnected() {
    if (WiFi.status() == WL_CONNECTED) return;
    unsigned long now = millis();
    if (now - lastWifiReconnectAttempt < 30000) return;
    lastWifiReconnectAttempt = now;
    Serial.println("Reconnexion Wi-Fi...");
    WiFi.disconnect();
    WiFi.begin(ssid, pass);
}

void afficherOLED() {
    oled.clear();
    
    // Affichage de l'heure en haut
    char heureOled[6];
    sprintf(heureOled, "%02d:%02d", current_dt.hour, current_dt.minute);
    oled.setCursor(0, 0); 
    oled.print(heureOled);
    
    // Témoin Wi-Fi
    if (WiFi.status() == WL_CONNECTED) {
        oled.setCursor(12, 0); oled.print("[W]");
    }
    
    // Affichage des pages rotatives
    switch (oledPage) {
        case 0: // Environnement (BME680 + DHT20)
            oled.setCursor(0, 2); oled.print("Env (BME|DHT)");
            oled.setCursor(0, 4); oled.print("T:"); oled.print(bme_temp, 1); oled.print("|"); oled.print(dht20_temp, 1);
            oled.setCursor(0, 5); oled.print("H:"); oled.print(bme_hum, 1); oled.print("|"); oled.print(dht20_hum, 1);
            oled.setCursor(0, 6); oled.print("P:"); oled.print(bme_pres, 0); oled.print(" hPa");
            break;
        case 1: // CO2 (SCD30 et MHZ16)
            oled.setCursor(0, 2); oled.print("Dioxyde Carbone");
            oled.setCursor(0, 4); oled.print("SCD : "); oled.print(scd_co2); oled.print(" ppm");
            oled.setCursor(0, 6); oled.print("MHZ : "); oled.print(mhz16_co2); oled.print(" ppm");
            break;
        case 2: // PM (HM3301)
            oled.setCursor(0, 2); oled.print("Particules(PM)");
            oled.setCursor(0, 4); oled.print("1.0 : "); oled.print(pm1_0);
            oled.setCursor(0, 5); oled.print("2.5 : "); oled.print(pm2_5);
            oled.setCursor(0, 6); oled.print("10  : "); oled.print(pm10);
            break;
        case 3: // Gaz (Multichannel + SGP40)
            oled.setCursor(0, 2); oled.print("Gaz & VOC");
            oled.setCursor(0, 4); oled.print("NO2:"); oled.print(valNO2); oled.print(" Et:"); oled.print(valEtOH);
            oled.setCursor(0, 5); oled.print("VOC:"); oled.print(valVOC); oled.print(" CO:"); oled.print(valCO);
            oled.setCursor(0, 7); oled.print("SGP VOC: "); oled.print(sgp40_voc);
            break;
        case 4: // SGP30 et HCHO
            oled.setCursor(0, 2); oled.print("SGP30 & HCHO");
            oled.setCursor(0, 4); oled.print("TVOC:"); oled.print(sgp30_tvoc); oled.print("ppb");
            oled.setCursor(0, 5); oled.print("eCO2:"); oled.print(sgp30_eco2); oled.print("ppm");
            oled.setCursor(0, 7); oled.print("HCHO:"); oled.print(hcho_ppm, 2); oled.print("ppm");
            break;
    }
}

// Fonction de lecture MH-Z16 via Serial1
bool readMHZ16() {
    byte data[9];
    while(mhz16.available()) mhz16.read();
    mhz16.write(cmd_get_sensor, sizeof(cmd_get_sensor));
    
    long startTime = millis();
    while(mhz16.available() < 9) {
        if(millis() - startTime > 150) return false;
    }
    
    for(int i = 0; i < 9; i++) data[i] = mhz16.read();
    
    byte checksum = 0;
    for (int i = 1; i < 8; i++) checksum += data[i];
    checksum = 255 - checksum + 1;
    
    if(checksum != data[8]) return false;
    
    mhz16_co2 = (int)data[2] * 256 + (int)data[3];
    return true;
}

// --- FONCTIONS RTC (PCF85063) ---
byte decToBcd(byte val) { return ( (val/10*16) + (val%10) ); }
byte bcdToDec(byte val) { return ( (val/16*10) + (val%16) ); }

void forcerHeureRTC(int year, int month, int day, int hour, int minute, int second) {
    Wire.beginTransmission(0x51); Wire.write(0x00); Wire.write(0x00); Wire.endTransmission();
    Wire.beginTransmission(0x51);
    Wire.write(0x04); Wire.write(decToBcd(second)); Wire.write(decToBcd(minute));
    Wire.write(decToBcd(hour)); Wire.write(decToBcd(day)); Wire.write(0); 
    Wire.write(decToBcd(month)); Wire.write(decToBcd(year - 2000));
    Wire.endTransmission();
}

CustomDateTime lireHeureRTC() {
    Wire.beginTransmission(0x51); Wire.write(0x04); Wire.endTransmission();
    Wire.requestFrom(0x51, 7); 
    CustomDateTime dt;
    dt.second = bcdToDec(Wire.read() & 0x7F);
    dt.minute = bcdToDec(Wire.read() & 0x7F);
    dt.hour   = bcdToDec(Wire.read() & 0x3F);
    dt.day    = bcdToDec(Wire.read() & 0x3F);
    Wire.read(); 
    dt.month  = bcdToDec(Wire.read() & 0x1F);
    dt.year   = bcdToDec(Wire.read()) + 2000;
    return dt;
}
