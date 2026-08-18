# Procédure d'Installation Complète : Station EASE (Version Wi-Fi)

Ce document décrit pas-à-pas comment configurer, compiler et lancer l'ensemble du projet EASE dans sa version réseau local (Wi-Fi). 
Contrairement à la version LoRa, ce projet utilise une seule carte Arduino qui se connecte directement à votre box Internet (ou partage de connexion) pour envoyer ses données en HTTP au serveur Python de votre ordinateur.

---

## 1. Préparation Matérielle et Logicielle (Arduino)

### A. Bibliothèques requises (à installer via le gestionnaire de l'IDE Arduino)
- **U8g2** (par oliver) : Pour l'écran OLED.
- **Grove - Laser PM2.5 Sensor HM3301** (par Seeed Studio) : Pour les particules fines.
- **SD** (bibliothèque Arduino officielle) : Pour le journal local sur carte.
- **DHT20** (par Rob Tillaart) : Pour le capteur de température/humidité.
- **Adafruit BME680**, **Adafruit SGP30** et **Adafruit SGP40**.
- **SparkFun SCD30**, **Grove Multichannel Gas Sensor V2** et **DHT20**.
- Le WSP2110 est lu sur l'entrée analogique `A0` : la bibliothèque SFA3x n'est pas utilisée.

### ⚠️ B. Modification cruciale de la bibliothèque HM3301 (Pour Arduino R4)
L'Arduino UNO R4 utilise une architecture 32 bits, ce qui fait planter la bibliothèque officielle Seeed Studio du capteur de particules HM3301 lors de la compilation (erreur : `u32 has not been declared`).
**Vous devez impérativement corriger le code source de la bibliothèque :**

1. Sur votre ordinateur, allez dans le dossier : `Documents\Arduino\libraries\Grove_-_Laser_PM2.5_Sensor_HM3301\src`
2. Ouvrez le fichier `Seeed_HM330X.h` avec un éditeur de texte.
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

## 2. Configuration et Téléversement du code Arduino

1. Ouvrez le fichier `Station_Complete_wifi.ino`.
2. Copiez `arduino_secrets.h.example` sous le nom `arduino_secrets.h`.
3. Renseignez `SECRET_SSID`, `SECRET_PASS` et `SERVER_ADDRESS` dans ce fichier local. Ne publiez jamais `arduino_secrets.h`.
4. Utilisez exactement le même jeton dans `arduino_secrets.h` et `server_secrets.bat`. Ces deux fichiers locaux sont ignorés par Git.

5. **Mise à l'heure du module RTC (PCF85063)** :
   - À cause d'une incompatibilité de registre, la mise à l'heure doit se faire manuellement.
   - Dans le `setup()` (vers la ligne 120), décommentez la ligne `forcerHeureRTC(...)` et inscrivez la date et l'heure actuelles (Année, Mois, Jour, Heure, Minute, Seconde).
   - Téléversez le code une première fois. La puce va alors mémoriser la date et l'heure.
   - **TRÈS IMPORTANT** : Re-commentez avec `//` cette ligne juste après, puis **téléversez le code une seconde fois**. Sinon, la station reviendra dans le passé à chaque redémarrage ou coupure de batterie !

6. La carte est désormais prête. Elle se connectera au Wi-Fi dès l'allumage et tentera automatiquement de se reconnecter après une coupure.

---

## 3. Lancement du Serveur Web (Python)

Sur votre ordinateur, vous devez lancer le serveur Web pour réceptionner les données envoyées par l'Arduino en HTTP (requête POST) et afficher le tableau de bord.

### A. Prérequis
Assurez-vous que **Python** est installé sur le PC. *(Lors de l'installation sous Windows, cochez bien "Add Python to PATH" si ce n'est pas fait).*

### B. Installation des dépendances
1. Ouvrez une Invite de commandes (CMD) ou PowerShell.
2. Exécutez la commande suivante pour installer les bibliothèques requises :
   ```bash
   python -m pip install -r requirements.txt
   ```
   *(Note: Pas besoin de la bibliothèque pyserial pour cette version Wi-Fi).*

### ⚠️ C. Le Pare-Feu Windows (Très important !)
Comme l'Arduino va tenter d'envoyer des données sur le **port 5000** de votre ordinateur à travers le réseau local, il est très probable que le pare-feu de Windows (ou votre antivirus) bloque la connexion par sécurité.
- Au premier lancement du script Python, Windows vous demandera peut-être via une pop-up si vous autorisez Python à communiquer sur le réseau. **Cliquez sur "Autoriser"**.
- Si l'Arduino indique "Erreur de connexion serveur" sur son écran OLED, vous devrez ouvrir les réglages du Pare-Feu Windows et créer manuellement une **règle de pare-feu entrante autorisant le Port TCP 5000**.

### D. Lancement
1. Double-cliquez sur `lancer_serveur_wifi.bat`. Il charge automatiquement le jeton depuis `server_secrets.bat`.
2. Pour un lancement manuel, définissez le même jeton avant de lancer Python :
   ```powershell
   $env:EASE_INGEST_TOKEN="LA_MEME_VALEUR"
   python data_wifi.py
   ```
3. Vérifiez l'état du serveur sur `http://localhost:5000/api/health` : `ingest_protected` doit valoir `true`.

Le serveur refuse par défaut toute collecte sans jeton. Pour un test local temporaire seulement, `EASE_ALLOW_UNAUTHENTICATED=1` permet de désactiver cette protection.

### E. Visualisation
Ouvrez votre navigateur web (Chrome, Edge, Firefox...) et rendez-vous sur la page locale :
**http://localhost:5000**

Dès que l'Arduino sera connecté au réseau Wi-Fi de la pièce, il enverra ses données toutes les 10 secondes et vous verrez les graphiques s'animer !

> **Interprétation des gaz :** le Multichannel Gas Sensor V2 fournit des indices qualitatifs, pas des concentrations réglementaires. Le WSP2110 utilise une estimation analogique dépendante de son étalonnage `R0`. Les couleurs ne remplacent donc pas une mesure certifiée, et les seuils journaliers/annuels doivent être évalués sur des moyennes adaptées.
