
#include <U8x8lib.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include "SparkFun_SCD30_Arduino_Library.h" // Bibliothèque pour le SCD30

// --- Configuration des modules ---

// 1. Écran OLED
U8X8_SSD1306_128X64_NONAME_HW_I2C oled(/* reset=*/ U8X8_PIN_NONE);

// 2. Capteur SCD30
SCD30 airSensor;
int co2Value;
float tempValue;
float humValue;

// 3. Carte SD
const int chipSelect = 4;
const char nomFichier[] = "scd30log.csv";

// --- Gestion temporelle (1 minute) ---
unsigned long previousMillis = 0;
const long interval = 10000; 

// Structure pour l'heure
struct CustomDateTime {
    int year, month, day, hour, minute, second;
};

void setup() {
    Serial.begin(115200); 
    Wire.begin(); 
    
    // Initialisation Écran
    oled.begin();
    oled.setPowerSave(0); 
    oled.setFont(u8x8_font_chroma48medium8_r); 
    oled.clear();
    oled.setCursor(0, 0); 
    oled.print("Demarrage...");
    delay(1000); 

    // Initialisation Horloge RTC (Mode Direct)
    oled.setCursor(0, 2);
    Wire.beginTransmission(0x51);
    if (Wire.endTransmission() != 0) {
        Serial.println("Erreur : RTC introuvable !");
        oled.print("Erreur RTC !");
        while (1) delay(10);
    }
    
    // ========================================================================
    // LIGNE À MODIFIER PUIS À SUPPRIMER APRÈS LE 1ER TÉLÉVERSEMENT
    // Format : (Année, Mois, Jour, Heure, Minute, Seconde)
    //forcerHeureRTC(2026, 6, 24, 20, 12, 0); 
    // ========================================================================

    oled.print("RTC OK");
    delay(1000);

    // Initialisation Capteur SCD30
    oled.setCursor(0, 3);
    if (airSensor.begin() == false) {
        Serial.println("Erreur : Capteur SCD30 introuvable !");
        oled.print("Erreur SCD30!");
        while (1) delay(10);
    }
    // Le SCD30 a besoin de quelques secondes pour s'initialiser
    delay(2000); 
    oled.print("SCD30 OK");
    delay(1000);

    // Initialisation Carte SD
    oled.setCursor(0, 4);
    if (!SD.begin(chipSelect)) {
        Serial.println("Erreur : Carte SD introuvable !");
        oled.print("Erreur SD !");
        while (1) delay(10); 
    }
    oled.print("SD OK");
    delay(1000);

    // Écriture de l'en-tête CSV
    File dataFile = SD.open(nomFichier, FILE_WRITE);
    if (dataFile) {
        if(dataFile.size() == 0) {
            dataFile.println("Date,Heure,CO2(ppm),Temperature(C),Humidite(%)");
        }
        dataFile.close();
    }
    
    // Première lecture immédiate
    effectuerLectureEtAffichage();
}

void loop() {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
        effectuerLectureEtAffichage();
    }
}

void effectuerLectureEtAffichage() {
    // Le SCD30 signale quand une nouvelle mesure est prête
    if (airSensor.dataAvailable()) {
        
        co2Value = airSensor.getCO2();
        tempValue = airSensor.getTemperature();
        humValue = airSensor.getHumidity();

        CustomDateTime now = lireHeureRTC();
        
        char dateStr[11];
        char heureStr[9];
        sprintf(dateStr, "%02d/%02d/%04d", now.day, now.month, now.year);
        sprintf(heureStr, "%02d:%02d:%02d", now.hour, now.minute, now.second);

        // Création de la ligne CSV (avec conversion des float en String avec 2 décimales)
        String ligneCSV = String(dateStr) + "," + String(heureStr) + "," + 
                          String(co2Value) + "," + String(tempValue, 2) + "," + String(humValue, 2);

        File dataFile = SD.open(nomFichier, FILE_WRITE);
        if (dataFile) {
            dataFile.println(ligneCSV);
            dataFile.close();
            Serial.println("Enregistre: " + ligneCSV); 
        } else {
            Serial.println("Erreur d'ecriture SD");
        }

        // Affichage OLED
        oled.clear();
        
        oled.setCursor(0, 0);
        char heureOled[6];
        sprintf(heureOled, "%02d:%02d", now.hour, now.minute);
        oled.print(heureOled);
        
        oled.setCursor(0, 2);
        oled.print("CO2 : "); oled.print(co2Value); oled.print(" ppm");
        
        oled.setCursor(0, 4);
        oled.print("Temp: "); oled.print(tempValue, 1); oled.print(" C");
        
        oled.setCursor(0, 6);
        oled.print("Hum : "); oled.print(humValue, 1); oled.print(" %");
        
    } else {
        Serial.println("SCD30 pas encore pret.");
    }
}

// --- FONCTIONS POUR MODULE GROVE HIGH PRECISION RTC (PCF85063) ---

byte decToBcd(byte val) {
  return ( (val/10*16) + (val%10) );
}

byte bcdToDec(byte val) {
  return ( (val/16*10) + (val%16) );
}

void forcerHeureRTC(int year, int month, int day, int hour, int minute, int second) {
    Wire.beginTransmission(0x51);
    Wire.write(0x00); 
    Wire.write(0x00); 
    Wire.endTransmission();

    Wire.beginTransmission(0x51);
    Wire.write(0x04); 
    Wire.write(decToBcd(second));
    Wire.write(decToBcd(minute));
    Wire.write(decToBcd(hour));
    Wire.write(decToBcd(day));
    Wire.write(0); 
    Wire.write(decToBcd(month));
    Wire.write(decToBcd(year - 2000));
    Wire.endTransmission();
}

CustomDateTime lireHeureRTC() {
    Wire.beginTransmission(0x51);
    Wire.write(0x04); 
    Wire.endTransmission();
    
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