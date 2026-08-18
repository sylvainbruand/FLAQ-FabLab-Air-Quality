# Procédure d'Installation Complète : Station EASE (Version LoRa)

Ce document décrit pas-à-pas comment configurer, compiler et lancer l'ensemble du projet EASE dans sa version de transmission sans-fil longue portée (LoRa). Le projet se découpe en deux parties matérielles (les deux cartes Arduino) et une partie logicielle (le serveur Python sur PC).

---

## 1. Préparation Matérielle et Logicielle (Arduino)

Le projet utilise **deux cartes Arduino UNO R4 WiFi**.
1. **L'Émetteur** : Embarque tous les capteurs environnementaux et envoie les données par ondes radio LoRa.
2. **Le Récepteur** : Connecté en USB à l'ordinateur, il réceptionne les ondes LoRa et transmet les données au PC via le port Série (COM).

### A. Bibliothèques requises (à installer via le gestionnaire de l'IDE Arduino)
- **U8g2** (par oliver) : Pour l'écran OLED.
- **Grove - Laser PM2.5 Sensor HM3301** (par Seeed Studio) : Pour les particules fines.
- **SD** (bibliothèque Arduino officielle) : Pour le journal local sur carte.
- **DHT20** (par Rob Tillaart) : Pour le capteur de température/humidité.
- **Adafruit BME680**, **Adafruit SGP30** et **Adafruit SGP40**.
- **SparkFun SCD30**, **Grove Multichannel Gas Sensor V2** et **DHT20**.
- Le WSP2110 est lu sur l'entrée analogique `A0` : la bibliothèque SFA3x n'est pas utilisée.
- **RadioHead** (par Mike McCauley) : Pour la communication des modules LoRa Grove. *(À installer manuellement depuis le fichier ZIP fourni par Seeed Studio si introuvable dans le gestionnaire).*

### ⚠️ B. Modification cruciale de la bibliothèque HM3301 (Pour Arduino R4)
L'Arduino UNO R4 utilise une architecture 32 bits (processeur Renesas), ce qui fait planter la bibliothèque officielle Seeed Studio du capteur de particules HM3301 lors de la compilation (erreur : `u32 has not been declared`).
**Vous devez impérativement corriger le code source de la bibliothèque :**

1. Sur votre ordinateur, allez dans le dossier : `Documents\Arduino\libraries\Grove_-_Laser_PM2.5_Sensor_HM3301\src`
2. Ouvrez le fichier `Seeed_HM330X.h` avec un éditeur de texte (Bloc-notes, VSCode, etc.).
3. Modifiez le début du fichier pour ajouter les trois lignes `typedef` suivantes juste après les `#include` :

```cpp
#ifndef _SEEED_HM330X_H_
#define _SEEED_HM330X_H_

#include <Arduino.h>
#include <Wire.h>

// --- AJOUT OBLIGATOIRE POUR COMPATIBILITE ARDUINO R4 (32 bits) ---
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
// ---------------------------------------------------------------

#define HM330X_DEFAULT_IIC_ADDR 0x40
// ... suite du code
```

---

## 2. Téléversement des codes Arduino

### L'Émetteur (Capteurs + LoRa)
1. Ouvrez le fichier `Station_Emetteur\Station_Emetteur.ino`.
2. Branchez le module LoRa sur le port **UART** du Shield Grove.
3. Branchez les autres capteurs sur le Hub I2C, et la carte SD sur le port SPI.
4. Sélectionnez la carte "Arduino UNO R4 WiFi" et son port COM.
5. **Mise à l'heure du module RTC (PCF85063)** :
   - À cause d'une incompatibilité de registre, les bibliothèques RTC standards ont été contournées dans ce projet.
   - Pour régler l'heure : Dans le `setup()` du code (vers la ligne 185), ajoutez la commande manuelle : `forcerHeureRTC(2026, 12, 31, 23, 59, 0);` (en remplaçant par l'année, mois, jour, heure, minute, seconde actuelle).
   - Téléversez le code une première fois. La puce va alors mémoriser la date et l'heure de façon permanente grâce à sa pile.
   - **TRÈS IMPORTANT** : Effacez (ou commentez avec `//`) cette ligne juste après, puis **téléversez le code une seconde fois**. Sinon, la station reviendra dans le passé à chaque redémarrage ou coupure de courant !
6. La carte Émetteur est désormais prête et peut être alimentée sur batterie ou secteur de manière autonome.

### Le Récepteur (LoRa -> PC)
1. Ouvrez le fichier `Station_Recepteur\Station_Recepteur.ino`.
2. Branchez le second module LoRa sur le port **UART** de ce second shield Grove.
3. Téléversez le code sur cette carte "Arduino UNO R4 WiFi".
4. **Laissez cette carte branchée en USB à votre ordinateur en permanence**. C'est elle qui fait le pont radio.

---

## 3. Lancement du Serveur Web (Python)

Sur l'ordinateur qui récupère les données (celui où le Récepteur est branché), vous devez lancer le serveur Web pour stocker les données (CSV) et afficher le tableau de bord.

### A. Prérequis
Assurez-vous que **Python** est installé sur le PC. *(Lors de l'installation de Python sous Windows, il est impératif de cocher la case "Add Python to PATH" si ce n'est pas déjà fait).*

### B. Installation des dépendances
1. Ouvrez une Invite de commandes (CMD) ou PowerShell.
2. Exécutez la commande suivante pour installer les bibliothèques requises :
   ```bash
   python -m pip install -r requirements.txt
   ```
   *(Waitress remplace le serveur de développement de Flask pour assurer une exécution très robuste en production).*

### C. Configuration du Port COM
1. Dans l'IDE Arduino, vérifiez le numéro de port COM sur lequel votre **Arduino Récepteur** est branché (ex: `COM5`).
2. Définissez le port sans modifier le code :
   ```powershell
   $env:EASE_SERIAL_PORT="COM5"
   ```
3. La valeur par défaut reste `COM5`. Le débit peut être changé avec `EASE_SERIAL_BAUD`.

### D. Lancement
1. **Fermez absolument le moniteur série de l'IDE Arduino** s'il est ouvert (sinon Python ne pourra pas lire le port USB : erreur "Accès refusé").
2. Dans votre Invite de commandes, déplacez-vous dans le dossier du projet :
   ```bash
   cd "C:\Users\sylva\Documents\! projet climat\station_complete_lora"
   ```
3. Lancez le serveur :
   ```bash
   python data_lora.py
   ```
4. Vous verrez le message : `Connecté au récepteur LoRa sur COM5`.

### E. Visualisation
Ouvrez votre navigateur web (Chrome, Edge, Firefox...) et rendez-vous sur la page locale :
**http://localhost:5000**

Vous verrez alors le tableau de bord EASE Air Quality Lab s'animer et se mettre à jour en direct dès que l'Émetteur envoie des paquets LoRa !

L'état technique est disponible sur **http://localhost:5000/api/health**. Le dashboard signale désormais une donnée comme ancienne lorsqu'aucune trame récente n'a été reçue.

> **Interprétation des gaz :** le Multichannel Gas Sensor V2 fournit des indices qualitatifs, pas des concentrations réglementaires. Le WSP2110 utilise une estimation analogique dépendante de son étalonnage `R0`. Les couleurs ne remplacent donc pas une mesure certifiée, et les seuils journaliers/annuels doivent être évalués sur des moyennes adaptées.
