# Vérification et mise en service

## Avant le premier démarrage

1. Contrôler toutes les alimentations et la masse commune.
2. Insérer une microSD 32 Go High Endurance formatée en FAT32.
3. Installer une pile fonctionnelle dans la RTC Grove.
4. Activer **OPI PSRAM** dans les options de la XIAO ESP32S3.
5. Téléverser `ease_xiao.ino` et ouvrir le moniteur série à 115 200 bauds.
6. Avant de raccorder D0, mesurer la sortie du pont WSP2110 et confirmer
   qu’elle reste sous 3,3 V.

Tant que le WSP2110 et son pont diviseur ne sont pas montés, conserver
`EASE_WSP2110_ENABLED 0`. D0 peut alors rester entièrement non connecté.

## Démarrage attendu

Avec le WSP2110 désactivé, le moniteur série doit indiquer l’état des quatre
capteurs actifs, du hub TCA9548A, de la RTC, de la
microSD et du Wi-Fi. Le HM3301 demande environ 30 secondes de stabilisation. Le
SGP40 détecte rapidement les variations, mais ses performances nominales
nécessitent une période de fonctionnement plus longue. Le WSP2110 demande au
moins 120 heures de chauffe avant l’étalonnage initial ; avant cela, sa valeur
sert uniquement à vérifier que la chaîne analogique répond.

Pour le HM3301, chaque acquisition est retentée jusqu'à trois fois. Après une
erreur, le firmware rouvre le TCA9548A et renvoie automatiquement la commande
I2C `0x88`. Le moniteur affiche `HM=1` lorsque la trame de 29 octets et son
checksum sont valides. Les valeurs PM employées sont celles prévues par le
HM3301 pour l'environnement atmosphérique.

Avant les messages des capteurs, le moniteur doit afficher :

```text
Hub I2C TCA9548A 0x70: OK (canaux 0xFF)
```

Sans configuration Wi-Fi :

1. se connecter au réseau `EASE-XIAO` ;
2. utiliser le mot de passe `ease-air` ;
3. ouvrir `http://192.168.4.1`.

Avec configuration Wi-Fi, utiliser l’adresse affichée sur la quatrième page de
l’OLED.

## Contrôles fonctionnels

- les valeurs du dashboard changent toutes les 10 secondes ;
- le graphe gaz montre trois axes ;
- le graphe et la carte WSP2110 indiquent clairement « HCHO estimé » en ppm ;
- le graphe climat montre deux axes ;
- `/api/health` répond avec la mémoire libre ;
- la microSD contient le fichier quotidien dans `/data` ;
- le statut RTC et le statut microSD sont verts dans le dashboard ;
- après un redémarrage, l’historique récent réapparaît.

## Test des événements

Pour éviter d’exposer volontairement les capteurs à une pollution dangereuse,
faire le premier test avec des seuils temporairement abaissés dans `config.h`.
Vérifier qu’après trois mesures au-dessus du seuil :

- l’indice passe au rouge ;
- le buzzer émet son motif sonore ;
- l’événement reste actif jusqu’au retour sous le seuil de réarmement ;
- une ligne est ajoutée à `/events.csv` à la fin ;
- le dashboard affiche durée, moyenne et maximum.

Restaurer impérativement les seuils normaux après le test.

## Étalonnage indicatif du WSP2110

1. Laisser le module fonctionner au moins 120 heures dans un air propre et
   stable, sans solvants, alcool ni fumée à proximité.
2. Relever plusieurs valeurs `WSP_Rs` dans le moniteur série et calculer leur
   moyenne.
3. Copier `config.h.example` vers `config.h`, puis remplacer
   `EASE_WSP2110_R0_RATIO` par cette moyenne.
4. Téléverser à nouveau le firmware et contrôler la stabilité pendant plusieurs
   heures.

Cette calibration suit la courbe typique du module Grove. Le WSP2110 répond à
plusieurs composés organiques ; ne pas utiliser sa valeur comme mesure
réglementaire ou comme dispositif de sécurité.

## Diagnostic rapide

- **PSRAM ERREUR sur OLED** : activer OPI PSRAM dans l’IDE.
- **microSD indisponible** : vérifier FAT32, CS sur D2 et les quatre fils SPI.
- **RTC à vérifier** : contrôler la pile, l’adresse `0x51` et remettre l’heure
  par NTP ou au moyen d’un sketch de réglage.
- **dashboard inaccessible** : lire l’adresse IP sur l’OLED et vérifier que le
  téléphone ou le PC se trouve sur le même réseau.
- **un capteur manque** : débrancher temporairement les autres branches du hub
  et vérifier le capteur seul avec un scanner I²C.
- **WSP2110 absent ou valeur incohérente** : contrôler son alimentation 5 V, la
  masse commune, le pont 10 kΩ / 20 kΩ et les valeurs `ADC`/`WSP_Rs` du moniteur
  série ; ce capteur n’apparaît pas dans un scanner I²C.
