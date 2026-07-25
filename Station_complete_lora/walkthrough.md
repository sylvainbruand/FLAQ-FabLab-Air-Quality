# Station Complète EASE : Architecture LoRa 📡

Ce document résume la migration complète de la station environnementale vers une architecture de communication radio **LoRa (868 MHz)** à double Arduino.

## Architecture Matérielle

L'architecture s'appuie désormais sur deux cartes **Arduino UNO R4** distinctes, communiquant entre elles par ondes radio, libérant ainsi la station extérieure de toute contrainte de couverture réseau Wi-Fi.

### 1. La Station Extérieure (L'Émetteur)
Située à l'extérieur, elle collecte les données de tous les capteurs environnementaux et les diffuse par radio.
* **Microcontrôleur :** Arduino UNO R4
* **Sauvegarde :** Carte SD locale (`all_log.csv`)
* **Communication :** Module Grove LoRa 868 MHz branché sur le port UART (TX/RX).
* **Capteurs I2C (Via Multiplexeur TCA9548A) :**
  * Entrée analogique A0 : WSP2110 (estimation HCHO)
  * Canal 2 : SGP30 (TVOC et eCO2)
  * Canal 3 : DHT20 (Température et Humidité)
  * Canal 4 : BME680 et SGP40 (adresses I2C distinctes)
  * Canal 5 : SCD30 (CO2, Temp, Hum)
  * Canal 6 : Multichannel Gas V2 (NO2, EtOH, VOC, CO)
  * Canal 7 : HM3301 (Particules fines PM1.0, 2.5, 10)
* **Capteurs Directs :** Écran OLED et Horloge RTC PCF85063.
* *(Note : Le capteur UART MH-Z16 a été supprimé pour libérer le port série matériel au profit du module LoRa).*

### 2. La Passerelle (Le Récepteur)
Située à l'intérieur, branchée en USB à votre ordinateur.
* **Microcontrôleur :** Arduino UNO R4
* **Communication :** Module Grove LoRa 868 MHz branché sur le port UART (TX/RX).
* **Rôle :** Écoute en permanence la fréquence 868 MHz. Lorsqu'un paquet radio est reçu et décodé, il est instantanément transféré à l'ordinateur via le câble USB (Serial).

## Architecture Logicielle

> [!TIP]
> L'architecture logicielle a été optimisée pour contourner les limitations de compatibilité du processeur Renesas (UNO R4). Le code force l'utilisation du template `HardwareSerial` pour la bibliothèque Grove LoRa, garantissant une compilation parfaite.

### Le Serveur Python (Backend)
Le fichier `data_lora.py` assure les fonctions suivantes :
1. **Écoute Série (Nouveauté) :** Il intègre un *thread* (processus en arrière-plan) utilisant la bibliothèque `pyserial`. Il écoute le port COM de l'Arduino récepteur en permanence.
2. **Serveur Web :** Il utilise toujours le framework *Flask* pour héberger le tableau de bord web.
3. **Mémorisation :** Chaque ligne valide est sauvegardée dans `all_sensors_lora.csv` selon le schéma commun à 23 colonnes. La colonne MH-Z16 est conservée pour compatibilité et vaut zéro lorsque ce capteur est absent.

### Le Tableau de Bord (Frontend)
Le fichier `dashboard.html` affiche les cartes et graphiques historiques. La carte MH-Z16 affiche un tiret lorsque le capteur est absent.

## Déploiement

Pour lancer le système :
1. Branchez l'Arduino récepteur au PC.
2. Définissez le port, par exemple dans PowerShell : `$env:EASE_SERIAL_PORT="COM5"`.
3. Lancez `data_lora.py` ou double-cliquez sur `lancer_serveur_lora.bat`.
4. Ouvrez votre navigateur sur `http://localhost:5000`.
