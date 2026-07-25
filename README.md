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

## Installation Python

Dans le dossier de la version choisie :

```powershell
python -m pip install -r requirements.txt
```

Utiliser ensuite le lanceur Windows fourni ou exécuter directement le serveur.

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
