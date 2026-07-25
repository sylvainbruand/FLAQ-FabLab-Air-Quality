#include <U8x8lib.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <Seeed_HM330X.h>

// --- Configuration des modules ---
U8X8_SSD1306_128X64_NONAME_HW_I2C oled(/* reset=*/ U8X8_PIN_NONE);

HM330X pmSensor;
uint8_t pmBuffer[30];
int pm1_0, pm2_5, pm10;

const int chipSelect = 4;
const char nomFichier[] = "air_data.csv";

unsigned long previousMillis = 0;
const long interval = 10000; // Mesure toutes les 60 secondes

// Structure pour stocker l'heure proprement
struct CustomDateTime {
    int year, month, day, hour, minute, second;
};

void setup() {
    Serial.begin(115200); 
    Wire.begin(); 
    
    // 1. Initialisation Écran
    oled.begin();
    oled.setPowerSave(0); 
    oled.setFont(u8x8_font_chroma48medium8_r); 
    oled.clear();
    oled.setCursor(0, 0); 
    oled.print("Demarrage...");
    delay(1000); 

    // 2. Initialisation Horloge RTC (Mode Direct)
    oled.setCursor(0, 2);
    Wire.beginTransmission(0x51);
    if (Wire.endTransmission() != 0) {
        Serial.println("Erreur : RTC introuvable a l'adresse 0x51 !");
        oled.print("Erreur RTC !");
        while (1) delay(10);
    }
    
    // ========================================================================
    // LIGNE À MODIFIER PUIS À SUPPRIMER APRÈS LE 1ER TÉLÉVERSEMENT
    // Format : (Année, Mois, Jour, Heure, Minute, Seconde)
    // Exemple pour le 24 Juin 2026 à 19h45 et 00s
    //forcerHeureRTC(2026, 6, 24, 19, 44, 0); 
    // ========================================================================

    oled.print("RTC OK");
    delay(1000);

    // 3. Initialisation Capteur PM
    oled.setCursor(0, 3);
    if (pmSensor.init()) {
        Serial.println("Erreur : Capteur HM3301 introuvable !");
        oled.print("Erreur HM3301!");
        while (1) delay(10);
    }
    oled.print("PM Sensor OK");
    delay(1000);

    // 4. Initialisation Carte SD
    oled.setCursor(0, 4);
    if (!SD.begin(chipSelect)) {
        Serial.println("Erreur : Carte SD introuvable !");
        oled.print("Erreur SD !");
        while (1) delay(10); 
    }
    oled.print("SD OK");
    delay(1000);

    // 5. Écriture de l'en-tête du fichier CSV
    File dataFile = SD.open(nomFichier, FILE_WRITE);
    if (dataFile) {
        if(dataFile.size() == 0) {
            dataFile.println("Date,Heure,PM1.0(ug/m3),PM2.5(ug/m3),PM10(ug/m3)");
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
    if (pmSensor.read_sensor_value(pmBuffer, 29) == NO_ERROR) {
        
        pm1_0 = (pmBuffer[10] << 8) | pmBuffer[11];
        pm2_5 = (pmBuffer[12] << 8) | pmBuffer[13];
        pm10  = (pmBuffer[14] << 8) | pmBuffer[15];

        CustomDateTime now = lireHeureRTC();
        
        char dateStr[11];
        char heureStr[9];
        sprintf(dateStr, "%02d/%02d/%04d", now.day, now.month, now.year);
        sprintf(heureStr, "%02d:%02d:%02d", now.hour, now.minute, now.second);

        char ligneCSV[60];
        sprintf(ligneCSV, "%s,%s,%d,%d,%d", dateStr, heureStr, pm1_0, pm2_5, pm10);

        File dataFile = SD.open(nomFichier, FILE_WRITE);
        if (dataFile) {
            dataFile.println(ligneCSV);
            dataFile.close();
            Serial.println(String("Enregistre: ") + ligneCSV); 
        } else {
            Serial.println("Erreur d'ecriture sur la SD");
        }

        oled.clear();
        oled.setCursor(0, 0);
        char heureOled[6];
        sprintf(heureOled, "%02d:%02d", now.hour, now.minute);
        oled.print(heureOled);
        
        oled.setCursor(0, 2);
        oled.print("PM1.0: "); oled.print(pm1_0);
        
        oled.setCursor(0, 4);
        oled.print("PM2.5: "); oled.print(pm2_5);
        
        oled.setCursor(0, 6);
        oled.print("PM10 : "); oled.print(pm10);
        
    } else {
        Serial.println("Erreur lecture capteur PM.");
        oled.clear();
        oled.setCursor(0, 0);
        oled.print("Erreur Capteur");
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
    // 1. Débloque l'horloge
    Wire.beginTransmission(0x51);
    Wire.write(0x00); 
    Wire.write(0x00); 
    Wire.endTransmission();

    // 2. Injecte l'heure au format BCD dans les bons registres
    Wire.beginTransmission(0x51);
    Wire.write(0x04); // Registre des secondes sur le PCF85063
    Wire.write(decToBcd(second));
    Wire.write(decToBcd(minute));
    Wire.write(decToBcd(hour));
    Wire.write(decToBcd(day));
    Wire.write(0); // Jour de la semaine ignoré
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
    Wire.read(); // Jour de la semaine ignoré
    dt.month  = bcdToDec(Wire.read() & 0x1F);
    dt.year   = bcdToDec(Wire.read()) + 2000;
    
    return dt;
}