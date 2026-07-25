# Interface Web R10.37 — supervision structurée

La version R10.37 renouvelle entièrement la présentation de la page Web, sans changer le protocole Web Serial ni le firmware Arduino. Elle n’ajoute donc aucune consommation de Flash ou de SRAM sur l’Arduino Uno.

## Interface

- navigation latérale fixe sur ordinateur, horizontale sur mobile ;
- vue plus large, avec cartes de mesures compactes et regroupées par logique de lecture ;
- sections distinctes : tableau de bord, journal SD, tolérances/fiabilité, configuration, montage et références ;
- aucun logo ou marque générique dans l’en-tête.

## Point important sur les mesures

- `CO2`: mesure NDIR en ppm via MH-Z16 ; la fiche fabricant donne, sur l’échelle 0–2000 ppm, ±(100 ppm + 6 % de la lecture), avec 3 minutes de préchauffage.
- `temperature_air_c` et `humidite_pct`: Si7021, ±0,4 °C et ±3 %RH selon Silicon Labs.
- `pression_hpa`: DPS310, pression relative ±0,06 hPa et absolue ±1 hPa, selon les conditions fabricant.
- `pm1_ug_m3`, `pm25_ug_m3`, `pm10_ug_m3`: HM3301, en µg/m³. La fiche indique 30 secondes de stabilisation et une cohérence ±10 µg/m³ de 0 à 100 µg/m³, puis ±10 % de 100 à 500 µg/m³, à 25 °C / 50 %RH.
- `tvoc_ppb`: SGP30, signal calculé par un algorithme de base interne. Le firmware actuel ne lui transmet pas l’humidité absolue, donc la compensation d’humidité optionnelle du SGP30 n’est pas active.
- `hcho_ppm_est`: estimation issue de la calibration R0 du montage ; le module HCHO à semi-conducteur est sensible à plusieurs COV. Le fabricant avertit que c’est une tendance approximative, pas une concentration exacte.
- `gas_no2_index` et `gas_co_index`: signaux relatifs 0–999, volontairement non libellés ppm/ppb.

## NO₂ et CO en ppm

Le module Gas Sensor v2 est documenté par Seeed comme adapté à une détection qualitative plutôt que quantitative. Les éléments GM102B et GM702B sont sensibles à plusieurs gaz et le firmware journalise des valeurs brutes plafonnées à 999. Il n’existe pas de conversion ppm traçable à partir de cette sortie dans le projet actuel.

Passer à une lecture ppm nécessiterait une nouvelle chaîne métrologique : sortie non tronquée, préchauffage contrôlé, gaz étalons à plusieurs concentrations, compensation température/humidité, caractérisation des sensibilités croisées, suivi de dérive et validation périodique par un appareil de référence. Pour la sécurité CO, utiliser un détecteur certifié.

## Compatibilité

- Chrome ou Edge pour Web Serial ;
- protocole inchangé : `PING`, `STATUS`, `GET_CONFIG`, `SET_NAME`, `SET_POS`, `SET_INTERVAL`, `SET_ROTATE`, `SET_RTC`, `SAVE_CONFIG`, `RESET_DATA CONFIRM` ;
- garder `index.html` et `serial.js` dans le même dossier.
