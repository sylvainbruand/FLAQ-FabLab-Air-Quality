// ==========================================
// = Projet EASE - Récepteur LoRa vers PC =
// ==========================================

/*
  Ce code est destiné au 2ème Arduino (le récepteur), branché en USB à l'ordinateur.
  Il utilise la bibliothèque officielle "Grove LoRa 433MHz and 915MHz RF" (qui fonctionne aussi pour 868MHz).
*/

#include <RH_RF95.h>

// On utilise le port matériel Serial1 de l'Arduino R4, en forçant le type HardwareSerial
#define COMSerial Serial1
RH_RF95<HardwareSerial> rf95((HardwareSerial&)COMSerial);

void setup() {
    Serial.begin(115200);
    
    // Le module Grove LoRa communique à 9600 baud par défaut
    COMSerial.begin(9600);
    delay(1000);
    
    Serial.println("Initialisation du Recepteur LoRa...");
    
    if (!rf95.init()) {
        Serial.println("Erreur: Impossible d'initialiser le module LoRa. Verifiez le cablage (TX/RX) !");
        while (1);
    }
    
    // IMPORTANT : On force la fréquence à 868 MHz (fréquence européenne)
    rf95.setFrequency(868.0);
    
    Serial.println("Recepteur LoRa 868MHz initialise. En attente de donnees...");
}

void loop() {
    if (rf95.available()) {
        uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
        uint8_t len = sizeof(buf);
        
        if (rf95.recv(buf, &len)) {
            // Convertir le buffer reçu en chaîne de caractères (String)
            String csvData = (char*)buf;
            
            // Sécurité : On s'assure de ne garder que la partie utile
            int nullPos = csvData.indexOf('\0');
            if(nullPos > -1) {
                csvData = csvData.substring(0, nullPos);
            }
            csvData.trim();
            
            if (csvData.length() > 10) { 
                // Envoi direct au PC pour le script Python
                Serial.println(csvData);
            }
        }
    }
}
