# Procedure d'Installation - Station LoRa Autonome (Sans Python)

Ce guide explique comment faire fonctionner la station **EASE LoRa de manière 100% autonome**, sans aucun ordinateur ni serveur Python actif.

---

## 💡 Principe du Système

Dans cette version :
1. **L'Arduino Émetteur** lit les capteurs, sauvegarde les données sur sa **Carte SD #1** (boîte noire de secours) et les émet par **LoRa (868MHz)**.
2. **L'Arduino Récepteur (UNO R4 WiFi)** reçoit les trames LoRa, enregistre les données sur sa propre **Carte SD #2** (`all_log.csv`) et **héberge son propre Serveur Web Wi-Fi** (Port 80).
3. Vous vous connectez à l'adresse IP de l'Arduino depuis n'importe quel ordinateur, tablette ou smartphone sur votre réseau Wi-Fi pour afficher le tableau de bord en direct et consulter l'historique !

---

## 🛠️ Matériel Requis

| Composant | Rôle | Broches / Connectique |
| :--- | :--- | :--- |
| **Arduino Émetteur (UNO R4)** | Station météo avec tous les capteurs | Shield Grove, LoRa sur `Serial1`, SD sur broche 4 |
| **Arduino Récepteur (UNO R4 WiFi)** | Récepteur radio, Carte SD & Serveur Web | Shield Grove, LoRa sur `Serial1`, SD sur broche 4, OLED sur I2C |
| **2x Cartes Micro-SD** | Formatées en **FAT32** | 1 pour l'émetteur, 1 pour le récepteur |
| **2x Modules Carte SD V4.4** | Lecteurs Micro-SD | Connectés sur les broches SPI (CS = Broche 4) |

---

## 📂 1. Préparation de la Carte SD du Récepteur

Avant d'insérer la carte SD dans le récepteur :
1. Insérez la Micro-SD du **Récepteur** dans votre PC.
2. Assurez-vous qu'elle est formatée en **FAT32**.
3. Copiez le contenu du dossier `sd_card_files/` à la racine de la carte SD :
   - `dash.htm` (et/ou `dashboard.html`)
   - Le dossier `static/` (contenant le logo et les images des partenaires)

> 💡 *Note : L'Arduino créera automatiquement le fichier `all_log.csv` dès qu'il recevra la première donnée LoRa.*

---

## 💻 2. Configuration et Téléversement des Codes Arduino

### A. Émetteur (`Station_Emetteur/Station_Emetteur.ino`)
1. Ouvrez l'IDE Arduino.
2. Ouvrez le fichier `station_complete_lora_no_python/Station_Emetteur/Station_Emetteur.ino`.
3. Branchez l'Arduino Émetteur via USB.
4. Sélectionnez la carte **Arduino UNO R4 WiFi** et son port COM.
5. Cliquez sur **Téléverser**.

### B. Récepteur (`Station_Recepteur/Station_Recepteur.ino`)
1. Ouvrez le fichier `station_complete_lora_no_python/Station_Recepteur/Station_Recepteur.ino`.
2. Tout en haut du fichier, renseignez vos identifiants Wi-Fi :
   ```cpp
   char ssid[] = "VOTRE_NOM_WIFI";      // Nom de votre Wi-Fi
   char pass[] = "VOTRE_MOT_DE_PASSE";  // Mot de passe de votre Wi-Fi
   ```
3. Branchez l'Arduino Récepteur via USB.
4. Sélectionnez la carte **Arduino UNO R4 WiFi** et son port COM.
5. Cliquez sur **Téléverser**.

---

## 🚀 3. Mise en Route et Visualisation

1. Insérez la Carte SD #1 dans l'Émetteur et la Carte SD #2 dans le Récepteur.
2. Alimentez l'Émetteur (sur batterie ou bloc secteur USB).
3. Alimentez le Récepteur (sur n'importe quel chargeur USB 5V).
4. Regardez l'écran OLED du Récepteur :
   - Il initialise le LoRa et la Carte SD.
   - Il se connecte à votre réseau Wi-Fi.
   - Une fois connecté, l'écran OLED affiche son adresse IP (ex: `192.168.1.45`).
5. Sur votre PC, smartphone ou tablette (connecté au même réseau Wi-Fi), ouvrez un navigateur web et tapez l'adresse IP :
   ```text
   http://192.168.1.45
   ```
   *(remplacez par l'IP réelle affichée sur l'écran OLED)*          

🎉 **Bravo !** Le Tableau de Bord s'affiche instantanément. Les données se rafraîchissent automatiquement dès qu'un paquet LoRa est émis !
