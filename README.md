# EASE — Station de qualité de l’air

EASE est une station environnementale multicapteurs basée sur Arduino UNO R4.
Elle mesure notamment la température, l’humidité, la pression, le CO₂, les
particules PM1/PM2.5/PM10, les COV et plusieurs indices de gaz.

## Architectures

### Wi‑Fi + serveur Python

La station envoie une trame CSV toutes les dix secondes par HTTP. Le serveur
Flask/Waitress valide les 23 champs, conserve l’historique et fournit le
dashboard sur le réseau local.

- Firmware : `Station_Complete_wifi/Station_Complete_wifi.ino`
- Serveur : `Station_Complete_wifi/data_wifi.py`
- Installation : `Station_Complete_wifi/PROCEDURE_INSTALLATION_WIFI.md`

### LoRa + passerelle Python

Un émetteur collecte les mesures et les transmet en LoRa 868 MHz. Un second
Arduino reçoit les trames et les transmet au serveur par USB.

- Émetteur : `Station_complete_lora/Station_Emetteur/`
- Récepteur : `Station_complete_lora/Station_Recepteur/`
- Serveur : `Station_complete_lora/data_lora.py`
- Installation : `Station_complete_lora/PROCEDURE_INSTALLATION_Lora.md`

### LoRa autonome

Cette variante héberge le dashboard directement sur un Arduino récepteur avec
stockage sur carte SD, sans serveur Python.

- Sources : `station_complete_lora_no_python/`

## Dashboard

Les versions Python proposent :

- valeurs en direct et indicateur de fraîcheur ;
- historique graphique limité et sous-échantillonné ;
- interface responsive et multilingue ;
- PWA avec cache local ;
- endpoint de santé `/api/health`.

Les sorties du Grove Multichannel Gas Sensor V2 sont présentées comme des
indices qualitatifs bruts. Elles ne sont pas assimilées à des concentrations
réglementaires.

## Installation sur un nouvel ordinateur Windows

Les versions LoRa et Wi‑Fi utilisent le même dashboard, mais pas le même mode
de transmission. Suivre uniquement le guide correspondant à la station à
installer.

> Les deux serveurs utilisent le port TCP `5000`. Ne pas lancer les versions
> LoRa et Wi‑Fi en même temps sur le même ordinateur.

### Préparation commune

1. Copier le projet complet sur le nouvel ordinateur, par exemple dans
   `C:\EASE`, ou le récupérer avec Git. Si le projet est cloné avec Git,
   installer également [Git LFS](https://git-lfs.com/) puis exécuter
   `git lfs pull` dans le dossier du projet.
2. Installer [Python 3 pour Windows](https://www.python.org/downloads/windows/).
   Pendant l'installation, cocher **Add Python to PATH**.
3. Vérifier Python dans PowerShell :

   ```powershell
   py -3 --version
   ```

4. Installer [Arduino IDE 2](https://www.arduino.cc/en/software).
5. Dans le gestionnaire de cartes de l'IDE Arduino, installer le paquet
   **Arduino UNO R4 Boards**.
6. Dans le gestionnaire de bibliothèques, installer les bibliothèques utilisées
   par la station :

   - U8g2 ;
   - Grove - Laser PM2.5 Sensor HM3301 ;
   - DHT20 ;
   - Adafruit BME680, Adafruit SGP30 et Adafruit SGP40 ;
   - SparkFun SCD30 ;
   - Grove Multichannel Gas Sensor V2 ;
   - RadioHead pour la version LoRa uniquement.

   La bibliothèque `SD` est fournie avec l'environnement Arduino.

7. Si la compilation signale `u32 has not been declared`, ouvrir :

   ```text
   Documents\Arduino\libraries\Grove_-_Laser_PM2.5_Sensor_HM3301\src\Seeed_HM330X.h
   ```

   Ajouter après les lignes `#include` :

   ```cpp
   typedef uint32_t u32;
   typedef uint16_t u16;
   typedef uint8_t u8;
   ```

### Guide pas à pas — station LoRa

La version LoRa utilise deux Arduino UNO R4 WiFi : un émetteur relié aux
capteurs et un récepteur LoRa relié en USB au PC.

#### 1. Préparer et téléverser l'émetteur

1. Brancher les capteurs et le module LoRa de l'émetteur. Le module LoRa doit
   être connecté au port **UART** du shield Grove.
2. Ouvrir
   `Station_complete_lora\Station_Emetteur\Station_Emetteur.ino`.
3. Dans l'IDE Arduino, sélectionner **Arduino UNO R4 WiFi**, puis le port COM de
   la carte.
4. Pour régler l'horloge RTC, ajouter temporairement dans `setup()` un appel
   avec la date et l'heure actuelles :

   ```cpp
   forcerHeureRTC(2026, 7, 26, 12, 0, 0);
   ```

5. Téléverser une première fois, supprimer ou commenter immédiatement cet
   appel, puis téléverser une seconde fois. Sans cette seconde étape, l'horloge
   reviendrait à la même date après chaque redémarrage.

#### 2. Préparer et téléverser le récepteur

1. Brancher le second module LoRa au port **UART** de l'autre shield Grove.
2. Ouvrir
   `Station_complete_lora\Station_Recepteur\Station_Recepteur.ino`.
3. Sélectionner **Arduino UNO R4 WiFi** et le port COM du récepteur.
4. Téléverser le programme.
5. Laisser ensuite ce récepteur connecté en USB au PC qui hébergera le serveur.

#### 3. Installer le serveur Python LoRa

1. Ouvrir PowerShell dans le dossier du projet.
2. Exécuter :

   ```powershell
   cd ".\Station_complete_lora"
   py -3 -m venv .venv
   .\.venv\Scripts\python.exe -m pip install --upgrade pip
   .\.venv\Scripts\python.exe -m pip install -r requirements.txt
   ```

3. Dans l'IDE Arduino ou le Gestionnaire de périphériques Windows, relever le
   port COM du récepteur, par exemple `COM5`.
4. Fermer le moniteur série de l'IDE Arduino : un seul programme peut ouvrir le
   port série à la fois.

#### 4. Configurer le port COM et démarrer

Si le récepteur est sur `COM5`, double-cliquer directement sur
`lancer_serveur_lora.bat`.

Pour utiliser un autre port, lancer le serveur depuis PowerShell :

```powershell
cd ".\Station_complete_lora"
$env:EASE_SERIAL_PORT = "COM7"
.\lancer_serveur_lora.ps1
```

Remplacer `COM7` par le port réellement affiché sur le nouvel ordinateur. Le
débit par défaut est `115200` bauds ; il peut être remplacé avec la variable
`EASE_SERIAL_BAUD` si le firmware est configuré différemment.

#### 5. Vérifier le fonctionnement

1. Attendre le message `Connecté au récepteur LoRa`.
2. Ouvrir [http://localhost:5000](http://localhost:5000).
3. Vérifier l'état technique sur
   [http://localhost:5000/api/health](http://localhost:5000/api/health).
4. Allumer l'émetteur et vérifier que les mesures se mettent à jour.
5. Pour arrêter le serveur, revenir dans son terminal et appuyer sur
   `Ctrl+C`.

En cas d'erreur `Accès refusé` sur le port série, fermer le moniteur série et
toute autre instance du serveur. En cas d'erreur `Port COM introuvable`,
contrôler à nouveau le numéro attribué au récepteur.

Le guide LoRa détaillé reste disponible dans
[`Station_complete_lora/PROCEDURE_INSTALLATION_Lora.md`](Station_complete_lora/PROCEDURE_INSTALLATION_Lora.md).

### Guide pas à pas — station Wi‑Fi

La version Wi‑Fi utilise un seul Arduino UNO R4 WiFi. L'Arduino et le PC serveur
doivent être connectés au même réseau local.

#### 1. Préparer le PC et connaître son adresse réseau

1. Connecter le PC au réseau Wi‑Fi qui sera utilisé par la station.
2. Ouvrir PowerShell et exécuter :

   ```powershell
   ipconfig
   ```

3. Dans la section de la carte Wi‑Fi active, noter l'**Adresse IPv4**, par
   exemple `192.168.1.100`. Cette adresse sera renseignée dans l'Arduino.
4. Éviter un réseau invité qui interdit les communications entre appareils.
   Si possible, réserver cette adresse IP au PC dans la box afin qu'elle ne
   change pas.

#### 2. Créer les deux fichiers secrets

1. Dans `Station_Complete_wifi`, copier les modèles :

   ```powershell
   cd ".\Station_Complete_wifi"
   Copy-Item ".\arduino_secrets.h.example" ".\arduino_secrets.h"
   Copy-Item ".\server_secrets.bat.example" ".\server_secrets.bat"
   ```

2. Ouvrir `arduino_secrets.h` et renseigner :

   ```cpp
   #define SECRET_SSID "NOM_DU_WIFI"
   #define SECRET_PASS "MOT_DE_PASSE_DU_WIFI"
   #define SERVER_ADDRESS "192.168.1.100"
   #define EASE_INGEST_TOKEN "UN_JETON_LONG_ET_ALEATOIRE"
   ```

3. Ouvrir `server_secrets.bat` et saisir exactement le même jeton :

   ```bat
   @echo off
   set "EASE_INGEST_TOKEN=UN_JETON_LONG_ET_ALEATOIRE"
   ```

4. Ne pas envoyer ni publier ces deux fichiers : ils contiennent le mot de
   passe Wi‑Fi et le jeton privé de la station.

#### 3. Téléverser le programme Wi‑Fi

1. Ouvrir `Station_Complete_wifi\Station_Complete_wifi.ino`.
2. Sélectionner **Arduino UNO R4 WiFi** et le port COM de la carte.
3. Pour régler l'horloge RTC, décommenter temporairement l'appel
   `forcerHeureRTC(...)` dans `setup()`, saisir la date et l'heure actuelles,
   puis téléverser une première fois.
4. Recommenter immédiatement cet appel et téléverser une seconde fois.
5. Laisser la station allumée et vérifier qu'elle se connecte au réseau Wi‑Fi.

#### 4. Installer le serveur Python Wi‑Fi

Dans PowerShell, depuis la racine du projet :

```powershell
cd ".\Station_Complete_wifi"
py -3 -m venv .venv
.\.venv\Scripts\python.exe -m pip install --upgrade pip
.\.venv\Scripts\python.exe -m pip install -r requirements.txt
```

#### 5. Autoriser le serveur dans le pare-feu

1. Double-cliquer sur `lancer_serveur_wifi.bat`.
2. Au premier lancement, si Windows demande une autorisation réseau pour
   Python, l'autoriser au minimum sur les **réseaux privés**.
3. Si aucune demande n'apparaît et que l'Arduino ne joint pas le serveur,
   créer dans le Pare-feu Windows une règle entrante autorisant le port
   **TCP 5000** sur le profil privé.

#### 6. Vérifier le fonctionnement

1. Ouvrir [http://localhost:5000](http://localhost:5000) sur le PC serveur.
2. Ouvrir
   [http://localhost:5000/api/health](http://localhost:5000/api/health) et
   vérifier que `ingest_protected` vaut `true`.
3. Attendre environ dix secondes et vérifier que les mesures de la station
   apparaissent.
4. Pour tester depuis un autre appareil du même réseau, ouvrir
   `http://ADRESSE_IP_DU_PC:5000`, par exemple
   `http://192.168.1.100:5000`.
5. Pour arrêter le serveur, revenir dans son terminal et appuyer sur
   `Ctrl+C`.

Si le dashboard s'ouvre mais qu'aucune mesure n'arrive, vérifier en priorité
l'adresse `SERVER_ADDRESS`, l'identité des deux jetons, le pare-feu, ainsi que
la connexion du PC et de l'Arduino au même réseau.

Le guide Wi‑Fi détaillé reste disponible dans
[`Station_Complete_wifi/PROCEDURE_INSTALLATION_WIFI.md`](Station_Complete_wifi/PROCEDURE_INSTALLATION_WIFI.md).

## Configuration et sécurité

Les identifiants Wi‑Fi, jetons de collecte, journaux CSV et fichiers propres à
la machine ne sont pas versionnés. Copier les fichiers `.example`, renseigner
les valeurs localement, puis téléverser le firmware.

## Tests

```powershell
python -m unittest discover -s tests -v
```

Les fichiers CAD, présentations et archives volumineuses sont suivis avec
Git LFS.
