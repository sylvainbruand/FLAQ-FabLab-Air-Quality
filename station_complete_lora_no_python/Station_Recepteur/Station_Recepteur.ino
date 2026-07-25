// ====================================================================
// = Projet EASE - Station LoRa Autonome Sans Python (Récepteur Web) =
// ====================================================================
// Matériel : Arduino UNO R4 WiFi + Shield Grove LoRa (Serial1) + Module SD (SPI CS=4) + OLED
// Description : Ce récepteur capte les trames LoRa de l'émetteur, les enregistre
//               sur sa carte SD locale et héberge un serveur Web Wi-Fi (port 80).
//               Permet de consulter le tableau de bord depuis tout PC/Smartphone.

#include <RH_RF95.h>
#include <WiFiS3.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <U8x8lib.h>

// --- Config Identifiants Wi-Fi ---
char ssid[] = "VOTRE_NOM_WIFI";      // <--- Renseignez le nom de votre réseau Wi-Fi
char pass[] = "VOTRE_MOT_DE_PASSE";  // <--- Renseignez le mot de passe Wi-Fi

// --- Configuration LoRa (Serial1 sur UNO R4) ---
#define COMSerial Serial1
RH_RF95<HardwareSerial> rf95((HardwareSerial&)COMSerial);

// --- Configuration Carte SD & OLED ---
const int SD_CS_PIN = 4;
const char LOG_FILE[] = "all_log.csv"; // Nom court 8.3 pour compatibilité SD FAT
U8X8_SSD1306_128X64_NONAME_HW_I2C oled(/* reset=*/ U8X8_PIN_NONE);

// --- Serveur Web ---
WiFiServer server(80);

// --- Stockage des dernières valeurs en direct (JSON) ---
String lastCsvLine = "";
unsigned long totalPackets = 0;
unsigned long lastPacketTime = 0;

void setup() {
    Serial.begin(115200);
    delay(1000);

    // Initialisation OLED
    oled.begin();
    oled.setPowerSave(0);
    oled.setFont(u8x8_font_chroma48medium8_r);
    oled.drawString(0, 0, "EASE Recepteur");
    oled.drawString(0, 1, "Init LoRa...");

    // 1. Initialisation LoRa
    COMSerial.begin(9600);
    if (!rf95.init()) {
        Serial.println(F("[ERREUR] Impossible d'initialiser le module LoRa !"));
        oled.drawString(0, 2, "LoRa ERROR !");
        while (1);
    }
    rf95.setFrequency(868.0);
    Serial.println(F("[OK] LoRa 868MHz pret."));
    oled.drawString(0, 2, "LoRa 868MHz OK");

    // 2. Initialisation Carte SD
    oled.drawString(0, 3, "Init SD...");
    if (!SD.begin(SD_CS_PIN)) {
        Serial.println(F("[AVERTISSEMENT] Carte SD non detectee ou erreur d'init. Les logs ne seront pas sauves."));
        oled.drawString(0, 3, "SD ERROR !");
    } else {
        Serial.println(F("[OK] Carte SD initialisee avec succes."));
        oled.drawString(0, 3, "SD Card OK");
    }

    // 3. Connexion Wi-Fi
    oled.drawString(0, 4, "Connexion WiFi..");
    Serial.print(F("Connexion au WiFi: "));
    Serial.println(ssid);

    WiFi.begin(ssid, pass);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        IPAddress ip = WiFi.localIP();
        Serial.println(F("\n[OK] WiFi Connecte !"));
        Serial.print(F("Adresse IP du Serveur Web : http://"));
        Serial.println(ip);

        oled.clear();
        oled.drawString(0, 0, "EASE Web Server");
        oled.drawString(0, 1, "IP:");
        char ipStr[16];
        sprintf(ipStr, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
        oled.drawString(0, 2, ipStr);
        oled.drawString(0, 4, "Attente LoRa...");
    } else {
        Serial.println(F("\n[ERREUR] Impossible de se connecter au WiFi."));
        oled.drawString(0, 4, "WiFi ECHEC !");
    }

    // 4. Lancement du Serveur HTTP
    server.begin();
    Serial.println(F("[OK] Serveur Web HTTP lance sur le port 80."));
}

void loop() {
    // --- 1. ÉCOUTE LORA ---
    if (rf95.available()) {
        uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
        uint8_t len = sizeof(buf);

        if (rf95.recv(buf, &len)) {
            String csvData = (char*)buf;
            int nullPos = csvData.indexOf('\0');
            if (nullPos > -1) {
                csvData = csvData.substring(0, nullPos);
            }
            csvData.trim();

            if (csvData.length() > 10) {
                totalPackets++;
                lastPacketTime = millis();
                lastCsvLine = csvData;

                Serial.print(F("[LoRa Recu #"));
                Serial.print(totalPackets);
                Serial.print(F("] "));
                Serial.println(csvData);

                // Sauvegarde sur Carte SD locale du récepteur
                File logFile = SD.open(LOG_FILE, FILE_WRITE);
                if (logFile) {
                    logFile.println(csvData);
                    logFile.close();
                }

                // Mise à jour de l'affichage OLED
                oled.drawString(0, 4, "Paquets Rcv:");
                char pktStr[10];
                sprintf(pktStr, "%lu", totalPackets);
                oled.drawString(12, 4, pktStr);
            }
        }
    }

    // --- 2. SERVEUR WEB HTTP ---
    WiFiClient client = server.available();
    if (client) {
        String requestLine = "";
        boolean currentLineIsBlank = true;

        while (client.connected()) {
            if (client.available()) {
                char c = client.read();
                if (requestLine.length() < 100) {
                    requestLine += c;
                }

                if (c == '\n' && currentLineIsBlank) {
                    // Fin des en-têtes HTTP, traiter la requête
                    handleHttpRequest(client, requestLine);
                    break;
                }
                if (c == '\n') {
                    currentLineIsBlank = true;
                } else if (c != '\r') {
                    currentLineIsBlank = false;
                }
            }
        }
        delay(1);
        client.stop();
    }
}

// --- TRAITEMENT DES REQUÊTES WEB ---
void handleHttpRequest(WiFiClient &client, String req) {
    if (req.indexOf("GET /api/live") >= 0) {
        // Renvoie la dernière ligne CSV reçue au format JSON
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: application/json");
        client.println("Access-Control-Allow-Origin: *");
        client.println("Connection: close");
        client.println();
        
        client.print("{\"raw\":\"");
        client.print(lastCsvLine);
        client.print("\",\"total_packets\":");
        client.print(totalPackets);
        client.println("}");
    }
    else if (req.indexOf("GET /all_log.csv") >= 0 || req.indexOf("GET /data.csv") >= 0) {
        // Sert le fichier CSV d'historique directement depuis la carte SD
        if (SD.exists(LOG_FILE)) {
            File f = SD.open(LOG_FILE, FILE_READ);
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/csv");
            client.println("Connection: close");
            client.println();
            while (f.available()) {
                client.write(f.read());
            }
            f.close();
        } else {
            client.println("HTTP/1.1 404 Not Found");
            client.println("Content-Type: text/plain");
            client.println();
            client.println("Fichier CSV non trouve sur la carte SD.");
        }
    }
    else {
        // Sert la page dashboard.html (ou dash.htm) depuis la carte SD
        const char* htmlFile = "dash.htm";
        if (!SD.exists(htmlFile)) {
            htmlFile = "dashboard.html";
        }

        if (SD.exists(htmlFile)) {
            File f = SD.open(htmlFile, FILE_READ);
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/html; charset=utf-8");
            client.println("Connection: close");
            client.println();
            
            byte buffer[64];
            while (f.available()) {
                int bytesRead = f.read(buffer, sizeof(buffer));
                client.write(buffer, bytesRead);
            }
            f.close();
        } else {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/html; charset=utf-8");
            client.println();
            client.println("<html><body><h2>EASE Station Recepteur</h2>");
            client.println("<p>Le fichier <b>dashboard.html</b> (ou <b>dash.htm</b>) est introuvable sur la carte SD.</p>");
            client.println("<p><b>Derniere mesure LoRa recue :</b></p><code>" + lastCsvLine + "</code>");
            client.println("</body></html>");
        }
    }
}
