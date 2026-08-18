# Document de Transmission du Projet (Handoff) - Station Météo & Qualité de l'Air EASE

**Date de mise à jour** : 25 Juillet 2026  
**Projet** : EASE - Station Environnementale Multicapteurs  
**Partenaires / Rôle** : Rectorat / Projet EASE  

---

## 1. 🎯 Présentation Générale

Le projet **EASE** est une station météo et d'analyse de la qualité de l'air multicapteurs basée sur l'écosystème **Arduino UNO R4 (WiFi)** et des capteurs de précision environnementale (Sensirion, Seeed Studio, Winsen).

Elle permet de mesurer la température, l'humidité, la pression atmosphérique, le CO₂, les particules fines (PM1.0, PM2.5, PM10), l'ozone (O₃), le dioxyde d'azote (NO₂), le formaldéhyde (HCHO) et divers composés organiques volatils (COV).

Le système a été développé sous **3 architectures complémentaires** pour répondre à différents besoins de terrain (mobilité, autonomie, supervision centralisée).

---

## 2. 🏗️ Architecture du Système (3 Déclinaisons)

```
                       ┌───────────────────────────────────────────┐
                       │    ÉMETTEUR / STATION CAPTEURS (UNO R4)   │
                       │ BME680, SCD30, HM3301, Gas V2, SGP40, ... │
                       └─────────────────────┬─────────────────────┘
                                             │
             ┌───────────────────────────────┼───────────────────────────────┐
             │ (Wi-Fi HTTP POST)             │ (LoRa 868 MHz Radio)          │ (LoRa 868 MHz Radio)
             ▼                               ▼                               ▼
┌─────────────────────────┐     ┌─────────────────────────┐     ┌─────────────────────────┐
│ 1. VERSION WI-FI        │     │ 2. VERSION LORA + PC    │     │ 3. VERSION LORA AUTONOME│
│ (Station_Complete_wifi) │     │ (station_complete_lora) │     │ (lora_no_python)        │
├─────────────────────────┤     ├─────────────────────────┤     ├─────────────────────────┤
│ • Arduino émet en HTTP  │     │ • Récepteur USB sur PC  │     │ • Récepteur R4 WiFi     │
│ • Serveur Python Flask  │     │ • Script Python         │     │ • Serveur Web embarqué  │
│ • Disque Dur PC (CSV)   │     │ • Disque Dur PC (CSV)   │     │ • Carte SD Récepteur    │
└────────────┬────────────┘     └────────────┬────────────┘     └────────────┬────────────┘
             │                               │                               │
             └───────────────────────────────┼───────────────────────────────┘
                                             ▼
                               ┌───────────────────────────┐
                               │ NAVIGATEUR WEB / APP PWA  │
                               │  & APP ANDROID NATIVE APK │
                               └───────────────────────────┘
```

### A. Version 1 : `Station_Complete_wifi`
- **Fonctionnement** : L'Arduino UNO R4 WiFi lit les capteurs, sauvegarde les données sur sa carte SD locale (boîte noire) et envoie chaque mesure via requête HTTP `POST /data` à un serveur Python Flask/Waitress.
- **Stockage** : Fichier `all_sensors_log.csv` sur le PC.
- **Accès** : Navigateur Web & Application Android à l'adresse `http://<IP_PC>:5000`.

### B. Version 2 : `station_complete_lora`
- **Fonctionnement** : L'Arduino Émetteur transmet les trames CSV par ondes radio **LoRa 868 MHz** (`Serial1`). L'Arduino Récepteur (branché en USB au PC) reçoit la trame et la retransmet au script Python `data_lora.py`.
- **Stockage** : Fichier `all_sensors_lora.csv` sur le PC + Carte SD Émetteur.
- **Accès** : Navigateur Web & Application Android à l'adresse `http://localhost:5000`.

### C. Version 3 : `station_complete_lora_no_python` (100% Autonome)
- **Fonctionnement** : **Sans aucun PC ni serveur Python**. L'Arduino Récepteur (UNO R4 WiFi) reçoit les trames LoRa, enregistre le fichier `all_log.csv` sur sa propre **Carte SD #2** et **héberge lui-même le serveur Web HTTP (Port 80)**.
- **Stockage** : Carte SD #1 (Émetteur) + Carte SD #2 (Récepteur).
- **Accès** : Directement sur l'IP du récepteur `http://<IP_ARDUINO_RECEPTRICE>/`.

### D. Application Mobile Android & PWA (`Android_App_EASE`)
- **Projet Android Native** : Projet Android Studio complet (`MainActivity.java` WebView).
- **PWA (Progressive Web App)** : Fichiers `manifest.json`, `sw.js` et icônes d'application intégrés sur tous les tableaux de bord pour une installation en 1-clic sur l'écran d'accueil Android.

---

## 🔌 3. Inventaire du Matériel & Câblage (Pinout)

### Shield Grove V2 (Émetteur)
- **Alimentation** : 5V USB (Secteur ou Batterie externe).
- **Écran OLED** : I2C Direct (`0x3C`).
- **Horloge RTC PCF85063** : I2C Direct (`0x51`).
- **Carte SD V4.4** : SPI Matériel (Broche `CS = 4`).
- **Bouton Écran OLED** : Broche numérique `D5` (Permet d'éteindre/allumer l'écran).
- **Ventilation 5V** : Emplacement dédié (Extinction possible de l'écran pour économiser la batterie).

### Multiplexeur I2C TCA9548A (`0x70`)
- **Port 1** : *(Réservé)*
- **Port 2** : Capteur SGP30 (`0x58`)
- **Port 3** : Capteur DHT20 (`0x38`)
- **Port 4** : Capteur BME680 (`0x77`)
- **Port 5** : Capteur SCD30 (`0x61`)
- **Port 6** : Multichannel Gas Sensor V2 (`0x08`)
- **Port 7** : Capteur Particules HM3301 (`0x40`)

### Ports Analogiques & UART
- **Broche A0** : Capteur HCHO WSP2110 (Lecture analogique ppm).
- **Port Serial1 (TX/RX)** : Module Radio Grove LoRa 868MHz (Baudrate 9600).

---

## 📊 4. Grille des Seuils de Référence (Qualité de l'Air)

Chaque carte de capteur du tableau de bord intègre un bouton d'information **`?`** et une **évaluation dynamique par code couleur** en temps réel :

| Polluant / Paramètre | Vert 🟢 (Bon) | Orange 🟡 (Acceptable / Info) | Orange Foncé 🟠 (Dégradé / Aérer) | Rouge / Marron 🔴 (Alerte / Danger) | Source / Référence |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **CO₂ (SCD30 / MH-Z16)** | < 800 ppm | 800 – 1000 ppm | 1000 – 1500 ppm | > 1500 ppm (Alerte ERP) / ≥ 5000 ppm (INRS) | HCSP / ASHRAE |
| **PM2.5 (HM3301)** | < 15 µg/m³ | 15 – 25 µg/m³ | 25 – 50 µg/m³ | > 50 – 75 µg/m³ | OMS 2021 / Norme UE |
| **PM10 (HM3301)** | < 45 µg/m³ | 45 – 50 µg/m³ | — | > 50 µg/m³ | OMS / UE |
| **NO₂ (Multichannel V2)** | < 25 µg/m³ | 25 – 40 µg/m³ | 40 – 200 µg/m³ | > 200 – 400 µg/m³ | OMS / France |
| **Formaldéhyde (HCHO)** | < 10 µg/m³ (~0.008 ppm) | 10 – 30 µg/m³ | — | > 30 µg/m³ (~0.024 ppm) | ANSES / France ERP |
| **Index VOC (SGP40)** | < 120 Idx | 120 – 220 Idx | — | > 220 Idx | Sensirion |
| **TVOC (SGP30)** | < 300 ppb | 300 – 1000 ppb | — | > 1000 ppb | Sensirion |
| **Température** | 18 – 24 °C | 15–18 °C / 24–28 °C | — | < 15 °C / > 28 °C | Confort thermique |
| **Humidité** | 40 – 60 % | 30–40 % / 60–70 % | — | < 30 % / > 70 % | Hygrométrie |

---

## 📂 5. Structure et Rôle des Fichiers

```text
! projet climat/
├── HANDOFF.md                             <-- Ce document récapitulatif
├── Android_App_EASE/                      <-- Projet Android Studio (App Native APK)
│   ├── app/src/main/AndroidManifest.xml   <-- Manifeste Android (Autorisations Internet)
│   ├── app/src/main/java/com/ease/station/MainActivity.java <-- WebView Android
│   ├── gradle.properties                  <-- Config AndroidX
│   └── build.gradle / settings.gradle     <-- Config de compilation Gradle
│
├── Station_Complete_wifi/                 <-- VERSION 1 : Wi-Fi + Python
│   ├── Station_Complete_wifi.ino          <-- Code Arduino UNO R4 WiFi
│   ├── data_wifi.py                       <-- Serveur Python Flask / Waitress (Port 5000)
│   ├── PROCEDURE_INSTALLATION_WIFI.md     <-- Guide pas-à-pas d'installation
│   ├── templates/dashboard.html           <-- Interface Web Responsive + PWA + Modales
│   └── static/                            <-- Logos, manifest.json, sw.js (PWA)
│
├── station_complete_lora/                 <-- VERSION 2 : LoRa + Python PC
│   ├── Station_Emetteur/                  <-- Code Arduino Émetteur capteurs + SD
│   ├── Station_Recepteur/                 <-- Code Arduino Récepteur USB -> PC
│   ├── data_lora.py                       <-- Serveur Python Flask / Waitress + Thread Serie
│   ├── lancer_serveur_lora.bat            <-- Script de lancement 1-clic Windows 11
│   ├── PROCEDURE_INSTALLATION_Lora.md     <-- Guide d'installation
│   ├── templates/dashboard.html           <-- Interface Web Responsive
│   └── static/                            <-- Assets & PWA
│
├── station_complete_lora_no_python/        <-- VERSION 3 : LoRa Autonome sans PC
│   ├── Station_Emetteur/                  <-- Code Arduino Émetteur
│   ├── Station_Recepteur/                 <-- Code Arduino Récepteur WiFi + SD + WebServer
│   ├── sd_card_files/                     <-- Fichiers à copier sur la carte SD du récepteur
│   │   ├── dash.htm / dashboard.html      <-- Web App autonome avec parser CSV JS
│   │   └── static/                        <-- Assets
│   └── PROCEDURE_INSTALLATION_NO_PYTHON.md <-- Guide d'installation autonome
│
└── presentation et images/                <-- Diaporama PPTX & Photos des boîtiers
    └── generate_pptx.py                   <-- Script Python de génération automatique du PPTX
```

---

## 🚀 6. Procédures de Lancement Rapidement Récapitulées

### Pour la Version Wi-Fi :
1. Téléverser `Station_Complete_wifi.ino` sur l'Arduino R4 WiFi (avec l'IP du PC).
2. Lancer `python data_wifi.py`.
3. Ouvrir `http://<IP_PC>:5000` sur navigateur ou app Android.

### Pour la Version LoRa (PC) :
1. Téléverser `Station_Emetteur.ino` sur l'émetteur et `Station_Recepteur.ino` sur le récepteur.
2. Double-cliquer sur `lancer_serveur_lora.bat`.
3. Ouvrir `http://localhost:5000`.

### Pour la Version LoRa Autonome (Sans Python) :
1. Copier le contenu de `sd_card_files/` sur la Carte SD du récepteur.
2. Téléverser `Station_Recepteur.ino` (avec les identifiants Wi-Fi).
3. Ouvrir l'IP affichée sur l'écran OLED du récepteur.

---

## 📋 7. État Actuel du Projet & Prochaines Étapes Possibles

- ✅ **Système d'affichage Web & Mobile** : Design responsive haut de gamme avec graphiques interactifs (2h, 3h, 4h, 24h, 1 sem, 1 mois).
- ✅ **Seuils & Modales de référence** : Intégrés avec bouton **`?`** et changement dynamique des couleurs de cartes.
- ✅ **Lancement automatique Windows 11** : Script `.bat` optimisé avec encodage UTF-8 et gestion des exceptions.
- ✅ **Version Autonome** : Complète sans Python avec double carte SD.
- 📌 **Étape Matérielle à venir** : Montage physique du ventilateur 5V à l'emplacement prévu (`emplacement ventilateur.jpg`).
