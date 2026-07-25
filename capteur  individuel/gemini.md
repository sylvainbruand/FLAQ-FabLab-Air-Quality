# Résumé du Projet "Antigravity" : Data Logger Environnemental Wi-Fi

Ce document retrace la conception, le dépannage et l'évolution d'une station de mesure environnementale basée sur un Arduino UNO R4 WiFi, utilisant l'écosystème matériel Seeed Studio Grove.

## 1. Architecture Matérielle de Base
* **Microcontrôleur :** Arduino UNO R4 WiFi.
* **Extensions (Empilement) :** * Grove Base Shield (en haut) pour connecter facilement les modules I2C/UART.
  * Grove SD Card Shield V4.4 (au milieu) pour l'enregistrement local en SPI (Broche CS : 4).
* **Affichage :** Écran Grove OLED 1.3" (SSD1315) en I2C. *Note : L'utilisation initiale d'un écran E-Ink a été écartée car inadaptée aux rafraîchissements en temps réel (risque de brûlure de l'écran).*
* **Horloge (RTC) :** Grove High Precision RTC.

## 2. Évolution et Tests des Capteurs
Le projet a fait l'objet de tests successifs avec plusieurs capteurs pour affiner les données environnementales :

1. **MH-Z16 (CO2 - UART) :**
   * Branché initialement par erreur sur un port I2C, nécessitait le port UART.
   * Problème de synchronisation résolu en remplaçant `SoftwareSerial` par le port matériel natif `Serial1` du R4 et en gérant correctement le buffer de lecture.
2. **HM3301 (Particules Fines PM1.0, PM2.5, PM10 - I2C) :**
   * Capteur laser ne nécessitant pas d'étalonnage logiciel, mais sensible à l'encrassement physique et à l'humidité (faux positifs).
   * **Correction de bibliothèque :** La compilation sur l'architecture 32 bits du R4 échouait (`u32 has not been declared`). Résolu en éditant le fichier `Seeed_HM330X.h` pour y ajouter les définitions `typedef uint32_t u32;` etc.
3. **SCD30 (CO2, Température, Humidité - I2C) :**
   * Capteur NDIR de haute précision. Intègre une fonction d'étalonnage automatique (ASC) nécessitant une exposition régulière à l'air libre (400 ppm) sur une période de 7 jours.
4. **Multichannel Gas Sensor V2 (NO2, Ethanol, VOC, CO - I2C) :**
   * Capteur à base d'oxyde métallique (MOX). Nécessite un temps de préchauffage de la plaque interne avant de fournir des données stabilisées.
5. **BME680 (Température, Humidité, Pression, VOC - I2C) :**
   * Le capteur final retenu. Mesure les composés organiques volatils (VOC) sous forme de résistance électrique (en Ohms). Une valeur élevée (ex: 50 kΩ) indique un air pur, une valeur basse indique une pollution.

## 3. Défis Techniques Résolus

### A. Le piège de l'Horloge RTC (PCF85063 vs PCF8563)
* **Symptôme :** Dates aberrantes (ex: 38ème jour du mois, 38 heures) et défilement des secondes à la place des heures.
* **Cause :** Puce PCF85063 à l'adresse `0x51` au lieu du classique DS3231. Les bibliothèques standard (RTClib Adafruit) lisaient les mauvais registres (décalage de mémoire entre la version 8563 et 85063).
* **Solution :** Contournement total des bibliothèques RTC. Création de fonctions I2C brutes (`Wire.read` et `Wire.write`) pour injecter et lire l'heure manuellement au format BCD dans les bons registres (les secondes démarrent au registre `0x04`).

### B. Enregistrement Automatique sur PC via Wi-Fi
* **Objectif :** Ne plus dépendre uniquement de la carte SD physique et envoyer les données en direct sur un ordinateur.
* **Solution Client (Arduino R4) :** Utilisation de la puce Wi-Fi intégrée pour générer des requêtes HTTP POST contenant les lignes CSV.
* **Solution Serveur (PC/Spyder) :** Création d'un script Python.
  * Utilisation initiale de **Flask**.
  * Migration vers **Waitress** (serveur WSGI de production) pour supprimer les avertissements de sécurité de Flask et assurer une écoute réseau stable et robuste sur le port 5000.

## 4. Architecture Logicielle Finale (Serveur Python)
Script permettant de réceptionner les données de la station Antigravity en direct sur le réseau local :

```python
from flask import Flask, request
from waitress import serve

app = Flask(__name__)
nom_fichier = "bme680_wifi_log.csv"

# Création de l'en-tête CSV
try:
    with open(nom_fichier, 'x') as f:
        f.write("Date,Heure,Temperature(C),Humidite(%),Pression(hPa),Gaz_VOC(Ohms)\n")
except FileExistsError:
    pass

@app.route('/data', methods=['POST'])
def receive_data():
    ligne_csv = request.data.decode('utf-8')
    with open(nom_fichier, 'a') as f:
        f.write(ligne_csv + '\n')
    return "Data OK", 200

if __name__ == '__main__':
    print("Serveur WSGI Antigravity en ecoute sur le port 5000...")
    serve(app, host='0.0.0.0', port=5000)