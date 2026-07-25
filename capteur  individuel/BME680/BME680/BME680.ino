
#include <U8x8lib.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>

// --- Configuration des modules ---

// 1. Écran OLED
U8X8_SSD1306_128X64_NONAME_HW_I2C oled(/* reset=*/ U8X8_PIN_NONE);

// 2. Capteur BME680
Adafruit_BME680 bme; 

// 3. Carte SD
const int chipSelect = 4;
const char nomFichier[] = "bme680.csv";

// --- Gestion temporelle (1 minute) ---
unsigned long previousMillis = 0;
const long interval = 60000; 

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
    // LIGNE À MODIFIER PUIS À METTRE EN COMMENTAIRE APRÈS LE 1ER TÉLÉVERSEMENT
    // Format : (Année, Mois, Jour, Heure, Minute, Seconde)
    //forcerHeureRTC(2026, 6, 25, 18, 55, 0); 
    // ========================================================================

    oled.print("RTC OK");
    delay(1000);

    // Initialisation Capteur BME680 (L'adresse I2C Grove est généralement 0x76)
    oled.setCursor(0, 3);
    if (!bme.begin(0x76)) {
        Serial.println("Erreur : Capteur BME680 introuvable !");
        oled.print("Erreur BME680!");
        while (1) delay(10);
    }
    
    // Configuration recommandée par Bosch pour le BME680
    bme.setTemperatureOversampling(BME680_OS_8X);
    bme.setHumidityOversampling(BME680_OS_2X);
    bme.setPressureOversampling(BME680_OS_4X);
    bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
    bme.setGasHeater(320, 150); // Chauffe la plaque à 320°C pendant 150ms

    oled.print("BME680 OK");
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
            dataFile.println("Date,Heure,Temperature(C),Humidite(%),Pression(hPa),Gaz_VOC(Ohms)");
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
    // Ordre de lancer une mesure au BME680
    if (!bme.performReading()) {
        Serial.println("Erreur de lecture du BME680");
        return;
    }

    CustomDateTime now = lireHeureRTC();
    
    char dateStr[11];
    char heureStr[9];
    sprintf(dateStr, "%02d/%02d/%04d", now.day, now.month, now.year);
    sprintf(heureStr, "%02d:%02d:%02d", now.hour, now.minute, now.second);

    // Création de la ligne CSV
    // bme.pressure est en Pascals, on divise par 100 pour l'avoir en hPa (hectopascals)
    String ligneCSV = String(dateStr) + "," + String(heureStr) + "," + 
                      String(bme.temperature, 2) + "," + 
                      String(bme.humidity, 2) + "," + 
                      String(bme.pressure / 100.0, 2) + "," + 
                      String(bme.gas_resistance / 1000.0, 2); // Gaz en Kilo-Ohms pour plus de lisibilité

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
    oled.print("Heure: ");
    oled.print(heureOled);
    
    oled.setCursor(0, 2);
    oled.print("T: "); oled.print(bme.temperature, 1); oled.print(" C");
    
    oled.setCursor(0, 3);
    oled.print("H: "); oled.print(bme.humidity, 1); oled.print(" %");
    
    oled.setCursor(0, 4);
    oled.print("P: "); oled.print(bme.pressure / 100.0, 0); oled.print(" hPa");
    
    oled.setCursor(0, 6);
    oled.print("VOC: "); oled.print(bme.gas_resistance / 1000.0, 1); oled.print(" kOhm");
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