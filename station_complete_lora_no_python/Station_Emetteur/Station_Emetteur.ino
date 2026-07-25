#include <U8x8lib.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>

#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>
#include <Adafruit_SGP40.h>
#include <Adafruit_SGP30.h>
#include "SparkFun_SCD30_Arduino_Library.h"
#include <Seeed_HM330X.h>
#include <Multichannel_Gas_GMXXX.h>
#include <DHT20.h>

// --- Modules de base ---
U8X8_SSD1306_128X64_NONAME_HW_I2C oled(/* reset=*/ U8X8_PIN_NONE);
const int chipSelect = 4;
const char nomFichier[] = "all_log.csv";

// --- Objets Capteurs ---
Adafruit_BME680 bme;
Adafruit_SGP40 sgp40;
Adafruit_SGP30 sgp30;
SCD30 scd30;
HM330X hm3301;
GAS_GMXXX<TwoWire> gas;
DHT20 dht20;

// --- Configuration LoRa sur UART Matériel ---
#include <RH_RF95.h>
#define COMSerial Serial1
RH_RF95<HardwareSerial> rf95((HardwareSerial&)COMSerial);

// (Le capteur MH-Z16 a été supprimé car remplacé par le SCD30)

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
// DHT20
float dht20_temp = 0.0, dht20_hum = 0.0;
// Multichannel Gas
int valNO2 = 0, valEtOH = 0, valVOC = 0, valCO = 0;

// --- Gestion temporelle ---
unsigned long previousMillisData = 0;
const long intervalData = 10000; // 10 secondes (modifiable pour préserver la bande passante LoRa)

unsigned long previousMillisSGP40 = 0;
const long intervalSGP40 = 1000; // 1 seconde (Obligatoire pour l'algorithme du SGP40)

unsigned long previousMillisOled = 0;
const long intervalOled = 4000;  // 4 secondes pour changer de page OLED
int oledPage = 0; 

// ==========================================
// =               SETUP                    =
// ==========================================
void setup() {
    pinMode(BUTTON_PIN, INPUT);
    Serial.begin(115200); 
    // Initialisation LoRa sur UART
    COMSerial.begin(9600);
    Wire.begin();
    delay(500);

    if (!rf95.init()) {
        Serial.println("Erreur LoRa Init");
    } else {
        rf95.setFrequency(868.0);
    }
    
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

    // Initialisation des capteurs (I2C)
    oled.clear(); oled.setCursor(0, 0); oled.print("Init Capteurs");

    // 1. BME680
    if (!bme.begin(0x76) && !bme.begin(0x77)) {
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
        oled.setCursor(0, 2); oled.print("SCD Error");
    } else {
        oled.setCursor(0, 2); oled.print("SCD OK");
    }

    // 3. HM3301 (Sur Hub Port 7)
    tcaselect(7);
    if (hm3301.init()) {
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
        oled.setCursor(9, 5); oled.print("S30 Err");
    } else {
        sgp30.IAQinit(); 
        oled.setCursor(9, 5); oled.print("S30 OK");
    }

    // Le capteur HCHO (WSP2110) est analogique et se branche sur le port A0 du Grove Base Shield.
    // Pas besoin d'initialisation I2C.

    delay(2000);

    // Initialisation Carte SD
    oled.clear(); oled.setCursor(0, 0);
    if (!SD.begin(chipSelect)) {
        oled.print("Erreur SD !");
        while (1) delay(10); 
    }
    oled.print("SD OK");
    
    File dataFile = SD.open(nomFichier, FILE_WRITE);
    if (dataFile) {
        if(dataFile.size() == 0) {
            dataFile.println("Date,Heure,Temp_BME(C),Hum_BME(%),Pression(hPa),VOC_BME(kOhms),CO2_SCD30(ppm),Temp_SCD30(C),Hum_SCD30(%),PM1.0,PM2.5,PM10,CO2_MHZ16(ppm),NO2,EtOH,VOC_Multi,CO,VOC_SGP40,Temp_DHT20(C),Hum_DHT20(%),TVOC_SGP30(ppb),eCO2_SGP30(ppm),HCHO(ppm)");
        }
        dataFile.close();
    }
    
    delay(1000);
    effectuerLectureEtAffichage(); 
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
        oledPage = (oledPage + 1) % 5; 
        afficherOLED();
    }
    
    // SGP40 doit absolument être lu toutes les secondes (1Hz) pour son algorithme
    if (currentMillis - previousMillisSGP40 >= intervalSGP40) {
        previousMillisSGP40 = currentMillis;
        tcaselect(4);
        sgp40_voc = sgp40.measureVocIndex(bme_temp, bme_hum);
    }
    
    // Lecture et envoi LoRa
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
    if (bme.performReading()) {
        bme_temp = bme.temperature;
        bme_hum = bme.humidity;
        bme_pres = bme.pressure / 100.0;
        bme_voc = bme.gas_resistance / 1000.0;
    }
    
    // Lecture SCD30
    tcaselect(5);
    if (scd30.dataAvailable()) {
        scd_co2 = scd30.getCO2();
        scd_temp = scd30.getTemperature();
        scd_hum = scd30.getHumidity();
    }
    
    // Lecture MH-Z16 supprimee
    
    // Lecture HM3301
    tcaselect(7);
    if (!hm3301.read_sensor_value(hm3301_buf, 29)) {
        pm1_0 = (uint16_t)hm3301_buf[4] << 8 | hm3301_buf[5];
        pm2_5 = (uint16_t)hm3301_buf[6] << 8 | hm3301_buf[7];
        pm10  = (uint16_t)hm3301_buf[8] << 8 | hm3301_buf[9];
    }
    
    // Lecture Multichannel Gas
    tcaselect(6);
    valNO2  = gas.measure_NO2();
    valEtOH = gas.measure_C2H5OH();
    valVOC  = gas.measure_VOC();
    valCO   = gas.measure_CO();
    
    // La lecture du SGP40 a été déplacée dans le loop à 1Hz
    // car son algorithme a besoin d'une lecture par seconde pour fonctionner.
    
    // Lecture DHT20
    tcaselect(3);
    dht20.read();
    dht20_temp = dht20.getTemperature();
    dht20_hum = dht20.getHumidity();
    
    // Lecture SGP30
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
    float R0 = 34.28; // Valeur d'étalonnage typique
    hcho_ppm = pow(10.0, ((log10(Rs / R0) - 0.0827) / (-0.4807)));
    
    // Formatage CSV (23 colonnes, avec MH-Z16)
    char dateStr[11], heureStr[9];
    sprintf(dateStr, "%02d/%02d/%04d", current_dt.day, current_dt.month, current_dt.year);
    sprintf(heureStr, "%02d:%02d:%02d", current_dt.hour, current_dt.minute, current_dt.second);
    
    String csvLine = String(dateStr) + "," + String(heureStr) + "," + 
                     String(bme_temp, 2) + "," + String(bme_hum, 2) + "," + String(bme_pres, 2) + "," + String(bme_voc, 2) + "," +
                     String(scd_co2) + "," + String(scd_temp, 2) + "," + String(scd_hum, 2) + "," +
                     String(pm1_0) + "," + String(pm2_5) + "," + String(pm10) + "," +
                     String(mhz16_co2) + "," + String(valNO2) + "," + String(valEtOH) + "," + String(valVOC) + "," + String(valCO) + "," + String(sgp40_voc) + "," +
                     String(dht20_temp, 2) + "," + String(dht20_hum, 2) + "," +
                     String(sgp30_tvoc) + "," + String(sgp30_eco2) + "," + String(hcho_ppm, 2);
    
    // Sauvegarde sur SD
    File dataFile = SD.open(nomFichier, FILE_WRITE);
    if (dataFile) {
        dataFile.println(csvLine);
        dataFile.close();
        Serial.println("SD OK");
    }
    
    // Envoi LoRa
    rf95.send((uint8_t*)csvLine.c_str(), csvLine.length() + 1);
    rf95.waitPacketSent();
    Serial.println("LoRa Tx: " + csvLine);
    
    afficherOLED();
}

void afficherOLED() {
    oled.clear();
    
    char heureOled[6];
    sprintf(heureOled, "%02d:%02d", current_dt.hour, current_dt.minute);
    oled.setCursor(0, 0); 
    oled.print(heureOled);
    oled.setCursor(10, 0); oled.print("[LoRa]");
    
    switch (oledPage) {
        case 0:
            oled.setCursor(0, 2); oled.print("Env (BME|DHT)");
            oled.setCursor(0, 4); oled.print("T:"); oled.print(bme_temp, 1); oled.print("|"); oled.print(dht20_temp, 1);
            oled.setCursor(0, 5); oled.print("H:"); oled.print(bme_hum, 1); oled.print("|"); oled.print(dht20_hum, 1);
            oled.setCursor(0, 6); oled.print("P:"); oled.print(bme_pres, 0); oled.print(" hPa");
            break;
        case 1:
            oled.setCursor(0, 2); oled.print("Dioxyde Carbone");
            oled.setCursor(0, 4); oled.print("SCD : "); oled.print(scd_co2); oled.print(" ppm");
            oled.setCursor(0, 6); oled.print("MHZ : Retire");
            break;
        case 2:
            oled.setCursor(0, 2); oled.print("Particules(PM)");
            oled.setCursor(0, 4); oled.print("1.0 : "); oled.print(pm1_0);
            oled.setCursor(0, 5); oled.print("2.5 : "); oled.print(pm2_5);
            oled.setCursor(0, 6); oled.print("10  : "); oled.print(pm10);
            break;
        case 3:
            oled.setCursor(0, 2); oled.print("Gaz & VOC");
            oled.setCursor(0, 4); oled.print("NO2:"); oled.print(valNO2); oled.print(" Et:"); oled.print(valEtOH);
            oled.setCursor(0, 5); oled.print("VOC:"); oled.print(valVOC); oled.print(" CO:"); oled.print(valCO);
            oled.setCursor(0, 7); oled.print("SGP VOC: "); oled.print(sgp40_voc);
            break;
        case 4:
            oled.setCursor(0, 2); oled.print("SGP30 & HCHO");
            oled.setCursor(0, 4); oled.print("TVOC:"); oled.print(sgp30_tvoc); oled.print("ppb");
            oled.setCursor(0, 5); oled.print("eCO2:"); oled.print(sgp30_eco2); oled.print("ppm");
            oled.setCursor(0, 7); oled.print("HCHO:"); oled.print(hcho_ppm, 2); oled.print("ppm");
            break;
    }
}

// Fonction de lecture MH-Z16 supprimee

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
