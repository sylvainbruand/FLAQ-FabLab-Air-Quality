#include <Wire.h>
#include "rgb_lcd.h" // Bibliothèque pour l'écran Grove

// --- Configuration de l'écran ---
rgb_lcd lcd;

// --- Configuration du capteur CO2 ---
#define sensor Serial1
const unsigned char cmd_get_sensor[] = {
    0xff, 0x01, 0x86, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x79
};

int temperature;
int CO2PPM;

void setup() {
    // Initialisation des ports série
    sensor.begin(9600);
    Serial.begin(115200);
    
    // Initialisation de l'écran (16 colonnes, 2 lignes)
    lcd.begin(16, 2);
    lcd.setRGB(255, 255, 255); // Ecran blanc au démarrage
    
    lcd.setCursor(0, 0); // (colonne 0, ligne 0)
    lcd.print("Demarrage du");
    lcd.setCursor(0, 1); // (colonne 0, ligne 1)
    lcd.print("Capteur CO2...");
    
    delay(3000); // Temps de chauffe/lecture initial
    lcd.clear();
}

void loop() {
    if(dataRecieve()) {
        // Affichage sur le moniteur série (pour debug)
        Serial.print("Temperature: ");
        Serial.print(temperature);
        Serial.print(" °C  |  CO2: ");
        Serial.print(CO2PPM);
        Serial.println(" ppm");

        // --- Affichage sur l'écran LCD ---
        // Ligne 1 : Température
        lcd.setCursor(0, 0);
        lcd.print("Temp : ");
        lcd.print(temperature);
        lcd.print(" C     "); // Les espaces servent à effacer les anciens caractères

        // Ligne 2 : CO2
        lcd.setCursor(0, 1);
        lcd.print("CO2  : ");
        lcd.print(CO2PPM);
        lcd.print(" ppm   ");

        // --- Gestion des couleurs de l'écran ---
        if (CO2PPM < 800) {
            lcd.setRGB(0, 255, 0); // Vert : Qualité d'air excellente
        } 
        else if (CO2PPM >= 800 && CO2PPM < 1500) {
            lcd.setRGB(255, 100, 0); // Orange/Jaune : Penser à aérer
        } 
        else {
            lcd.setRGB(255, 0, 0); // Rouge : Qualité d'air mauvaise
        }

    } else {
        Serial.println("Erreur de lecture du capteur.");
        lcd.setRGB(255, 0, 0); // Rouge
        lcd.setCursor(0, 0);
        lcd.print("Erreur lecture  ");
        lcd.setCursor(0, 1);
        lcd.print("Verifier cables ");
    }
    
    delay(2000); // Attendre 2 secondes entre chaque mesure
}

// Fonction de lecture du capteur (identique à la version corrigée précédente)
bool dataRecieve(void) {
    byte data[9];
    
    // Vider le buffer
    while(sensor.available()) {
        sensor.read();
    }
 
    // Transmettre la requête
    sensor.write(cmd_get_sensor, sizeof(cmd_get_sensor));
    
    // Attendre la réponse complète avec timeout
    long startTime = millis();
    while(sensor.available() < 9) {
        if(millis() - startTime > 150) { 
            return false;
        }
    }
 
    // Lire les 9 octets
    for(int i = 0; i < 9; i++) {
        data[i] = sensor.read();
    }
    
    // Calcul et vérification du Checksum
    byte checksum = 0;
    for (int i = 1; i < 8; i++) {
        checksum += data[i];
    }
    checksum = 255 - checksum;
    checksum += 1;

    if(checksum != data[8]) {
        return false;
    }
    
    // Extraction des valeurs
    CO2PPM = (int)data[2] * 256 + (int)data[3];
    temperature = (int)data[4] - 40;
 
    return true;
}