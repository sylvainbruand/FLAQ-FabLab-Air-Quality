#include <U8x8lib.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include "RTClib.h"

// --- Configuration des modules ---

// 1. Écran OLED
U8X8_SSD1306_128X64_NONAME_HW_I2C oled(/* reset=*/ U8X8_PIN_NONE);

// 2. Horloge RTC (PCF8563 / PCF85063 à l'adresse 0x51)
//RTC_PCF8563 rtc;

// 3. Carte SD (La broche CS par défaut du Grove SD Shield V4 est la broche 4)
const int chipSelect = 4;
const char nomFichier[] = "mesures.csv";

// 4. Capteur CO2 (UART sur R4)
#define sensor Serial1
const unsigned char cmd_get_sensor[] = {
    0xff, 0x01, 0x86, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x79
};
int temperature;
int CO2PPM;

// --- Gestion temporelle (1 minute) ---
unsigned long previousMillis = 0;
const long interval = 10000; // interval mesure en ms

void setup() {
    sensor.begin(9600);   
    Serial.begin(115200); 
    
    // Initialisation Écran
    oled.begin();
    oled.setPowerSave(0); 
    oled.setFont(u8x8_font_chroma48medium8_r); 
    oled.clear();
    oled.setCursor(0, 0); 
    oled.print("Demarrage...");
    delay(1000); // Petite pause pour lire l'écran

   // Initialisation Horloge RTC (Mode Direct Wire)
    oled.setCursor(0, 2);
    Wire.begin(); // Initialise le bus I2C
    
    // On vérifie manuellement si la puce répond à l'adresse 0x51
    Wire.beginTransmission(0x51);
    if (Wire.endTransmission() != 0) {
        Serial.println("Erreur : RTC introuvable a l'adresse 0x51 !");
        oled.print("Erreur RTC !");
        while (1) delay(10); // Bloque le programme si le RTC n'est pas branché
    }
    
    // Initialisation Horloge RTC (Mode Direct Wire)
    oled.setCursor(0, 2);
    Wire.begin(); // Initialise le bus I2C
    
    // On vérifie manuellement si la puce répond à l'adresse 0x51
    Wire.beginTransmission(0x51);
    if (Wire.endTransmission() != 0) {
        Serial.println("Erreur : RTC introuvable a l'adresse 0x51 !");
        oled.print("Erreur RTC !");
        while (1) delay(10); // Bloque le programme si le RTC n'est pas branché
    }
    
    // ON FORCE la mise à l'heure du PCF85063 (Mode Direct)
    // ATTENTION : Une fois que le module est à l'heure, tu pourras supprimer
    // ou commenter les deux lignes ci-dessous pour qu'il vive sa vie sur sa pile !
    Serial.println("Forcage de la mise a l'heure du RTC...");
    forcerHeureRTC(DateTime(F(__DATE__), F(__TIME__))); 

    oled.print("RTC OK");
    delay(1000);
    
    // Initialisation Carte SD
    oled.setCursor(0, 4);
    if (!SD.begin(chipSelect)) {
        Serial.println("Erreur : Carte SD introuvable !");
        oled.print("Erreur SD !");
        while (1) delay(10); // Bloque le programme si pas de SD
    }
    oled.print("SD OK");
    delay(1000);

    // Écriture de l'en-tête du fichier CSV si le fichier n'existe pas encore
    File dataFile = SD.open(nomFichier, FILE_WRITE);
    if (dataFile) {
        // Ajoute l'en-tête seulement si le fichier est vide (taille = 0)
        if(dataFile.size() == 0) {
            dataFile.println("Date,Heure,CO2(ppm),Temperature(C)");
        }
        dataFile.close();
    } else {
        Serial.println("Erreur d'ouverture du fichier CSV !");
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
    if(dataRecieve()) {
        // 1. Récupération de l'heure exacte
        // On utilise notre propre fonction de lecture
        DateTime now = lireHeureRTC();
        
        // Formatage de la date (JJ/MM/AAAA) et de l'heure (HH:MM:SS)
        char dateStr[11];
        char heureStr[9];
        sprintf(dateStr, "%02d/%02d/%04d", now.day(), now.month(), now.year());
        sprintf(heureStr, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());

        // 2. Création de la ligne CSV (Date,Heure,CO2,Temp)
        String ligneCSV = String(dateStr) + "," + String(heureStr) + "," + String(CO2PPM) + "," + String(temperature);

        // 3. Sauvegarde sur la carte SD
        File dataFile = SD.open(nomFichier, FILE_WRITE);
        if (dataFile) {
            dataFile.println(ligneCSV);
            dataFile.close();
            Serial.println("Enregistre: " + ligneCSV); // Affiche dans le moniteur série pour debug
            //Serial.println(now.hour());
            //Serial.println(now.minute());
        } else {
            Serial.println("Erreur d'ecriture sur la SD");
        }

        // 4. Affichage OLED mis à jour
        oled.clear();
        
        // Ligne 1 : Heure (format HH:MM)
        oled.setCursor(0, 0);
        char heureOled[6];
        sprintf(heureOled, "%02d:%02d", now.hour(), now.minute());
        oled.print("Heure: ");
        oled.print(heureOled);
        
        // Ligne 3 : CO2
        oled.setCursor(0, 2);
        oled.print("CO2 : ");
        oled.print(CO2PPM);
        
        // Ligne 5 : Température
        oled.setCursor(0, 4);
        oled.print("Temp: ");
        oled.print(temperature);
        oled.print(" C");
        
    } else {
        Serial.println("Erreur capteur CO2.");
        oled.clear();
        oled.setCursor(0, 0);
        oled.print("Erreur Capteur");
    }
}
 
// Fonction de communication avec le capteur (Inchangée)
bool dataRecieve(void) {
    byte data[9];
    while(sensor.available()) sensor.read();
 
    sensor.write(cmd_get_sensor, sizeof(cmd_get_sensor));
    
    long startTime = millis();
    while(sensor.available() < 9) {
        if(millis() - startTime > 150) return false;
    }
 
    for(int i = 0; i < 9; i++) {
        data[i] = sensor.read();
    }
    
    byte checksum = 0;
    for (int i = 1; i < 8; i++) checksum += data[i];
    checksum = 255 - checksum;
    checksum += 1;

    if(checksum != data[8]) return false;
    
    CO2PPM = (int)data[2] * 256 + (int)data[3];
    temperature = (int)data[4] - 40;
 
    return true;
}
// --- FONCTIONS POUR MODULE GROVE HIGH PRECISION RTC (PCF85063) ---

byte decToBcd(byte val) {
  return ( (val/10*16) + (val%10) );
}

byte bcdToDec(byte val) {
  return ( (val/16*10) + (val%16) );
}

void forcerHeureRTC(DateTime dt) {
    // 1. On s'assure que l'horloge tourne (On désactive le bit STOP)
    Wire.beginTransmission(0x51);
    Wire.write(0x00); 
    Wire.write(0x00); 
    Wire.endTransmission();

    // 2. On injecte les données avec le bon décalage (Secondes dans le registre 0x04)
    Wire.beginTransmission(0x51);
    Wire.write(0x04); 
    Wire.write(decToBcd(dt.second()));
    Wire.write(decToBcd(dt.minute()));
    Wire.write(decToBcd(dt.hour()));
    Wire.write(decToBcd(dt.day()));
    Wire.write(0); // Jour de la semaine (ignoré)
    Wire.write(decToBcd(dt.month()));
    Wire.write(decToBcd(dt.year() - 2000));
    Wire.endTransmission();
}

DateTime lireHeureRTC() {
    Wire.beginTransmission(0x51);
    Wire.write(0x04); // On commence la lecture au registre 0x04
    Wire.endTransmission();
    
    Wire.requestFrom(0x51, 7); 
    int ss = bcdToDec(Wire.read() & 0x7F);
    int mm = bcdToDec(Wire.read() & 0x7F);
    int hh = bcdToDec(Wire.read() & 0x3F);
    int d  = bcdToDec(Wire.read() & 0x3F);
    Wire.read(); // Jour de la semaine (ignoré)
    int m  = bcdToDec(Wire.read() & 0x1F);
    int y  = bcdToDec(Wire.read()) + 2000;
    
    return DateTime(y, m, d, hh, mm, ss);
}