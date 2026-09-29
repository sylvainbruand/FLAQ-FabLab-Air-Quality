# FLAQ XIAO — FabLab Air Quality

Projet académique de contrôle de la qualité de l’air dans les FabLabs, réalisé
avec une **Seeed Studio XIAO ESP32S3**. La carte acquiert les
capteurs, affiche les mesures sur OLED, enregistre l’historique sur microSD et
héberge elle-même un dashboard accessible depuis un téléphone ou un ordinateur.

## Fonctions incluses

- actualisation des mesures toutes les 10 secondes ;
- lecture du SGP40 toutes les secondes avec compensation DHT20 ;
- trois graphiques avec axes indépendants ;
- moyenne glissante PM2.5 sur 24 heures ;
- indice intérieur FLAQ du vert au rouge ;
- alarme confirmée sur buzzer passif au niveau rouge ;
- journal persistant des dépassements avec durée, moyenne et maximum ;
- synthèse quotidienne et export CSV depuis le dashboard ;
- horodatage par RTC Grove PCF85063 ;
- correction NTP automatique lorsque le réseau donne accès à Internet ;
- point d’accès `FLAQ-XIAO` si aucun Wi-Fi n’est configuré ou disponible ;
- reprise de l’historique des dernières 24 heures après redémarrage.
- navigation manuelle entre les quatre pages OLED avec le bouton Grove ;
- extinction de l’OLED après 20 secondes sans appui et réveil automatique.
- déclaration manuelle d’une ventilation avec un second bouton Grove ;
- mode mute basculé en appuyant sur les deux boutons en même temps ;
- signalement de la ventilation sur le dashboard et éclaircissement des graphes.
- bandes temporelles claires sur les graphes pendant les périodes ventilées.

## Matériel

| Fonction | Référence | Connexion |
|---|---|---|
| Carte | Seeed Studio XIAO ESP32S3 | — |
| Hub | Grove 8-Channel I²C Hub TCA9548A | `D4/SDA`, `D5/SCL`, adresse `0x70` |
| CO₂ | SCD30 | I²C `0x61` |
| Particules | HM3301 | I²C `0x40` |
| VOC | SGP40 | I²C `0x59` |
| HCHO indicatif | Grove HCHO WSP2110 | analogique `D0`, via pont diviseur |
| Température/humidité | DHT20 | I²C `0x38` |
| Horloge | RTC Grove PCF85063 | I²C `0x51` |
| Affichage | OLED SSD1306 128×64 | I²C `0x3C` |
| Alarme | Grove Passive Buzzer 107020109 | `D1` |
| Navigation OLED + ventilation | Grove Dual Button | port `D7 / UART` (`D7` + `D6`) |
| Stockage | Adafruit MicroSD Breakout+ 254 | SPI, CS sur `D2` |

Le câblage détaillé se trouve dans [CABLAGE.md](CABLAGE.md).

Au démarrage, le firmware ouvre automatiquement les huit canaux du TCA9548A.
Cette configuration est possible car chaque capteur utilise une adresse I²C
différente. Elle se règle avec `FLAQ_I2C_MUX_*` dans `config.example.h`.

## Préparation de l’Arduino IDE

1. Installer le paquet de cartes **esp32 by Espressif Systems**.
2. Sélectionner **XIAO_ESP32S3**.
3. Activer la PSRAM en mode **OPI PSRAM**.
4. Sélectionner une taille Flash de **8 MB**.
5. Installer les bibliothèques :
   - U8g2 ;
   - Adafruit SGP40 Sensor, avec ses dépendances ;
   - SparkFun SCD30 Arduino Library ;
   - DHT20 de Rob Tillaart.

Le HM3301 est lu directement par le firmware via `Wire`, afin d’éviter
l’incompatibilité du type `u32` présente dans la bibliothèque Seeed 1.0.2 avec
les versions ESP32 récentes. En cas de trame incomplète ou de checksum invalide,
la lecture est retentée trois fois et le capteur est automatiquement replacé en
mode I2C. Les concentrations utilisées sont les valeurs « environnement
atmosphérique » du HM3301.

Les bibliothèques `WiFi`, `WebServer`, `ESPmDNS`, `SD`, `SPI` et `Wire` sont
fournies par le paquet ESP32.

## Configuration Wi-Fi

Le projet fonctionne immédiatement sans identifiants : la carte crée le réseau
`FLAQ-XIAO`, protégé par le mot de passe `flaq-air`. Une fois connecté à ce
réseau, ouvrir `http://192.168.4.1`.

Pour connecter la station au réseau local :

1. copier `config.h.example` sous le nom `config.h` ;
2. renseigner les noms et mots de passe des réseaux Wi-Fi dans
   `FLAQ_WIFI_NETWORKS` (une ligne par réseau) ;
3. téléverser à nouveau le programme.

Au démarrage, l’OLED affiche la liste configurée. Le bouton sur D7 passe au
réseau suivant et le bouton sur D6 valide le réseau affiché. Après la
validation, les deux boutons reprennent automatiquement leur fonctionnement
normal (pages OLED et ventilation). Si la connexion choisie échoue, la station
crée comme auparavant le point d’accès `FLAQ-XIAO`.

Le dashboard devient alors accessible à l’adresse IP affichée sur l’OLED et,
si le réseau l’autorise, à `http://flaq-xiao.local`.

`config.h` est ignoré par Git pour ne pas publier le mot de passe.

## Carte microSD

Utiliser une microSD High Endurance de 32 Go formatée en FAT32. La station crée :

- `/data/AAAAMMJJ.csv` : mesures d’une journée ;
- `/events.csv` : dépassements terminés.

La fenêtre glissante est maintenue en PSRAM. Au redémarrage, les fichiers du
jour courant et du jour précédent sont relus afin de reconstruire les dernières
24 heures, la moyenne PM2.5 et les périodes de ventilation. La colonne
`ventilation` vaut `1` lorsque la fenêtre ou la porte a été déclarée ouverte.

## Seuils

Tous les seuils se trouvent dans `config.example.h` et peuvent être redéfinis
dans `config.h`. L’indice global correspond au paramètre instantané le plus
défavorable parmi CO₂, VOC, HCHO estimé, PM2.5 et PM10.

Une alerte ou un événement nécessite trois mesures consécutives au-dessus du
seuil. Le retour utilise un seuil plus bas et trois confirmations, ce qui évite
les oscillations autour d’une limite.

Le seuil de la moyenne PM2.5 sur 24 heures produit un événement distinct. Il
n’est activé que lorsque la station dispose d’au moins 23 heures d’historique.

Ces seuils constituent un indicateur pédagogique FLAQ de qualité de l’air
intérieur ; ils ne remplacent pas un dispositif réglementaire ou certifié.

## Particularités du WSP2110

Le WSP2110 est lu sur D0 mais reste optionnel. S’il n’est pas branché, ou tant
qu’aucune lecture valide n’est détectée, le dashboard affiche « indisponible »,
le HCHO est ignoré par l’indice et aucune alerte HCHO n’est créée. Cette absence
n’empêche ni le démarrage ni le fonctionnement des autres capteurs.

Le WSP2110 est un capteur MOS sensible à plusieurs gaz et solvants. La valeur
« HCHO estimé » en ppm sert donc à suivre une tendance après étalonnage ; ce
n’est pas une mesure sélective ou certifiée du formaldéhyde. Le capteur demande
au moins 120 heures de préchauffage avant son étalonnage initial.

Le module Grove fonctionne en 5 V alors que l’entrée analogique du XIAO est en
3,3 V. Sa sortie doit obligatoirement passer par le pont 10 kΩ / 20 kΩ décrit
dans `CABLAGE.md`. La constante `FLAQ_WSP2110_R0_RATIO` doit ensuite être
ajustée dans `config.h` avec la valeur obtenue en air propre stabilisé. Le
moniteur série affiche `WSP_Rs` et `ADC` pour faciliter cette opération.

## API embarquée

| Route | Rôle |
|---|---|
| `/` | dashboard |
| `/logo.png` | logo FLAQ et favicon |
| `/api/live` | dernière mesure et état technique |
| `/api/history?since=...&max=900` | historique sous-échantillonné |
| `/api/events?max=200` | journal des dépassements |
| `/api/events.csv` | téléchargement CSV |
| `/api/health` | mémoire et disponibilité du serveur |

## Fichiers du projet

- `flaq_xiao.ino` : firmware complet ;
- `dashboard.h` : interface web embarquée sans dépendance Internet ;
- `flaq_logo.png` et `flaq_logo.h` : logo optimisé et ressource embarquée ;
- `config.example.h` : valeurs par défaut et seuils ;
- `config.h.example` : modèle de configuration Wi-Fi ;
- `CABLAGE.md` : raccordement pas à pas ;
- `VERIFICATION.md` : procédure de mise en service.
