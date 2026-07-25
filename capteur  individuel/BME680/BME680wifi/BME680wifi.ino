#include <U8x8lib.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>
#include <WiFiS3.h> // Bibliothèque spécifique au Wi-Fi du R4

// --- Configuration Réseau Wi-Fi ---
const char ssid[] = "VOTRE_RESEAU_WIFI"; // A MODIFIER
const char pass[] = "VOTRE_MOT_DE_PASSE_WIFI"; // A MODIFIER
const char serverAddress[] = "192.168.0.116"; // À MODIFIER : L'adresse IP de ton PC
const int serverPort = 5000;
WiFiClient client;

// --- Configuration des modules ---
U8X8_SSD1306_128X64_NONAME_HW_I2C oled(/* reset=*/ U8X8_PIN_NONE);
Adafruit_BME680 bme; 
const int chipSelect = 4;
const char nomFichier[] = "bme680.csv";

unsigned long previousMillis = 0;
const long interval = 60000; 

struct CustomDateTime {
    int year, month, day, hour, minute, second;
};

void setup() {
    Serial.begin(115200); 
    Wire.begin(); 
    
    oled.begin();
    oled.setPowerSave(0); 
    oled.setFont(u8x8_font_chroma48medium8_r); 
    oled.clear();
    oled.setCursor(0, 0); 
    oled.print("Demarrage...");

    // Initialisation Horloge RTC
    Wire.beginTransmission(0x51);
    if (Wire.endTransmission() != 0) {
        Serial.println("Erreur RTC !");
        while (1) delay(10);
    }
    
    // ========================================================================
    // Comme pour le code précédent, gère ta mise à l'heure ici en 2 temps !
    // forcerHeureRTC(2026, 6, 25, 19, 15, 0); 
    // ========================================================================

    // Connexion Wi-Fi
    oled.setCursor(0, 2);
    oled.print("Connexion WiFi..");
    Serial.print("Connexion a ");
    Serial.println(ssid);
    WiFi.begin(ssid, pass);
    
    // Attente de la connexion (avec timeout pour ne pas bloquer si pas de wifi)
    int wifiTimeout = 0;
    while (WiFi.status() != WL_CONNECTED && wifiTimeout < 10) {
        delay(1000);
        Serial.print(".");
        wifiTimeout++;
    }
    
    oled.setCursor(0, 3);
    if (WiFi.status() == WL_CONNECTED) {
        oled.print("WiFi OK!");
        Serial.println("\nWiFi connecte. IP: ");
        Serial.println(WiFi.localIP());
    } else {
        oled.print("WiFi Echoue :(");
        Serial.println("\nEchec Wi-Fi. Mode hors-ligne SD uniquement.");
    }
    delay(1500);

    // Initialisation BME680
    oled.clear(); oled.setCursor(0, 0);
    if (!bme.begin(0x76)) {
        oled.print("Erreur BME680!");
        while (1) delay(10);
    }
    bme.setTemperatureOversampling(BME680_OS_8X);
    bme.setHumidityOversampling(BME680_OS_2X);
    bme.setPressureOversampling(BME680_OS_4X);
    bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
    bme.setGasHeater(320, 150);

    // Initialisation SD
    if (!SD.begin(chipSelect)) {
        oled.print("Erreur SD !");
        while (1) delay(10); 
    }
    File dataFile = SD.open(nomFichier, FILE_WRITE);
    if (dataFile) {
        if(dataFile.size() == 0) {
            dataFile.println("Date,Heure,Temperature(C),Humidite(%),Pression(hPa),Gaz_VOC(Ohms)");
        }
        dataFile.close();
    }
    
    effectuerLectureEtAffichage();
}

void loop() {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
        effectuerLectureEtAffichage();
    }
}

void envoyerDonneesWiFi(String csvLine) {
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Envoi WiFi vers serveur Python...");
        if (client.connect(serverAddress, serverPort)) {
            // Requête HTTP POST
            client.println("POST /data HTTP/1.1");
            client.println("Host: " + String(serverAddress));
            client.println("Content-Type: text/plain");
            client.print("Content-Length: ");
            client.println(csvLine.length());
            client.println();
            client.print(csvLine); // On envoie notre ligne CSV pure
            
            client.stop(); // Ferme la connexion
            Serial.println("Envoi reussi.");
        } else {
            Serial.println("Erreur: Serveur Python inaccessible.");
        }
    }
}

void effectuerLectureEtAffichage() {
    if (!bme.performReading()) return;

    CustomDateTime now = lireHeureRTC();
    char dateStr[11], heureStr[9];
    sprintf(dateStr, "%02d/%02d/%04d", now.day, now.month, now.year);
    sprintf(heureStr, "%02d:%02d:%02d", now.hour, now.minute, now.second);

    String ligneCSV = String(dateStr) + "," + String(heureStr) + "," + 
                      String(bme.temperature, 2) + "," + 
                      String(bme.humidity, 2) + "," + 
                      String(bme.pressure / 100.0, 2) + "," + 
                      String(bme.gas_resistance / 1000.0, 2);

    // 1. Sauvegarde sur carte SD locale
    File dataFile = SD.open(nomFichier, FILE_WRITE);
    if (dataFile) {
        dataFile.println(ligneCSV);
        dataFile.close();
        Serial.println("SD OK: " + ligneCSV); 
    }

    // 2. Envoi des données via Wi-Fi
    envoyerDonneesWiFi(ligneCSV);

    // 3. Affichage OLED
    oled.clear();
    oled.setCursor(0, 0); oled.print(heureStr);
    
    // Icône Wi-Fi improvisée si connecté
    if (WiFi.status() == WL_CONNECTED) {
        oled.setCursor(12, 0); oled.print("[W]");
    }
    
    oled.setCursor(0, 2); oled.print("T: "); oled.print(bme.temperature, 1); oled.print(" C");
    oled.setCursor(0, 3); oled.print("H: "); oled.print(bme.humidity, 1); oled.print(" %");
    oled.setCursor(0, 4); oled.print("P: "); oled.print(bme.pressure / 100.0, 0); oled.print(" hPa");
    oled.setCursor(0, 6); oled.print("VOC: "); oled.print(bme.gas_resistance / 1000.0, 1); oled.print(" kOhm");
}

// --- FONCTIONS RTC (Inchangées) ---
byte decToBcd(byte val) { return ( (val/10*16) + (val%10) ); }
byte bcdToDec(byte val) { return ( (val/16*10) + (val%16) ); }

void forcerHeureRTC(int year, int month, int day, int hour, int minute, int second) {
    Wire.beginTransmission(0x51);
    Wire.write(0x00); Wire.write(0x00); Wire.endTransmission();
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
    dt.second = bcdToDec(Wire.read() & 0x7F); dt.minute = bcdToDec(Wire.read() & 0x7F);
    dt.hour   = bcdToDec(Wire.read() & 0x3F); dt.day    = bcdToDec(Wire.read() & 0x3F);
    Wire.read(); 
    dt.month  = bcdToDec(Wire.read() & 0x1F); dt.year   = bcdToDec(Wire.read()) + 2000;
    return dt;
}