# Interface Web R10.36 — console environnementale

Cette version remplace entièrement la page `index.html` et conserve le protocole Web Serial de R10.35 : aucune modification du firmware Arduino n'est nécessaire. Elle n'augmente donc ni la mémoire Flash ni la RAM de l'Arduino Uno.

## Points importants

- Interface responsive, sans bibliothèque externe ni accès réseau obligatoire pour l'affichage.
- Les liens de documentation fabricant s'ouvrent séparément lorsque le navigateur a accès à Internet.
- Les mesures sont présentées selon leur nature : mesure, estimation ou signal relatif.
- Les valeurs NO2/CO sont volontairement libellées **signal relatif** : le firmware lit des valeurs ADC des canaux GM102B et GM702B, limitées à 999 pour le journal. Elles ne doivent pas être appelées ppm ou ppb.
- La page Web lit le cache du firmware via `STATUS`. Elle ne force ni lecture capteur ni accès carte SD.

## Compatibilité

- Chrome ou Edge, via Web Serial.
- Même protocole et mêmes commandes que R10.35 : `PING`, `STATUS`, `GET_CONFIG`, `SET_NAME`, `SET_POS`, `SET_INTERVAL`, `SET_ROTATE`, `SET_RTC`, `SAVE_CONFIG`, `RESET_DATA CONFIRM`.
- Ouvrir `Projet_EASE_Web/index.html` après avoir décompressé l'archive. Ne pas ouvrir le Moniteur série Arduino en même temps.

## Documenter un vrai NO2/CO en ppm

La conversion ne doit pas être improvisée. Elle nécessite au minimum : signaux non tronqués, préconditionnement, essais avec gaz étalons à plusieurs concentrations, modèles de compensation température/humidité, étude des sensibilités croisées, et contrôles périodiques face à une référence. Pour un usage avec seuils de sécurité ou décision réglementaire, employer une chaîne de mesure dédiée et calibrée.
