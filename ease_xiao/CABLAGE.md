# Câblage de la station FLAQ XIAO

## Bus I²C Grove

Raccorder le Grove 8-Channel I²C Hub TCA9548A au port I²C du Grove Shield XIAO :

- SDA : `D4 / GPIO5` ;
- SCL : `D5 / GPIO6` ;
- alimentation et masse communes.

Brancher ensuite sur le hub : SCD30, HM3301, SGP40, DHT20, OLED et RTC
PCF85063. Leurs adresses sont différentes ; aucun multiplexeur logiciel n’est
nécessaire par capteur. Le firmware écrit `0xFF` à l’adresse `0x70` au démarrage
pour ouvrir simultanément les huit canaux du TCA9548A.

La longueur totale des câbles I²C doit rester raisonnable. Le firmware utilise
100 kHz pour améliorer la robustesse avec plusieurs modules.

## Buzzer Grove 107020109

Brancher le buzzer passif sur le port numérique correspondant à `D1`. Le
firmware produit une tonalité de 2 700 Hz lorsque l’indice rouge est confirmé.

## Grove Dual Button sur D7 / UART

Raccorder le Grove Dual Button au connecteur marqué `D7 / UART` du shield
XIAO. Les deux lignes de signal de cette prise sont utilisées :

| Signal Grove | XIAO ESP32S3 | Fonction |
|---|---|---|
| jaune / SIG1 | `D7` | bouton de changement de page |
| blanc / SIG2 | `D6` | bouton de ventilation |
| rouge | `3,3 V` | alimentation du Dual Button |
| noir | `GND` | masse |

Le bouton sur D7 fait avancer d’une page et la quatrième page revient à la première.
L’écran s’éteint après 20 secondes sans appui ; l’appui suivant le rallume et
affiche directement la page suivante. Le filtrage anti-rebond est intégré au
firmware.

Le bouton sur D6 inverse l’état : ventilation active, puis ventilation arrêtée.
Un appui simultané sur les deux boutons active ou désactive le mode muet.
Les sorties du module sont actives à l’état bas ; le firmware active aussi les
résistances de tirage internes de la XIAO.

Le Dual Button doit être alimenté ici en 3,3 V afin que ses niveaux hauts restent
sans danger pour la XIAO. Ne jamais envoyer directement un signal 5 V sur D6 ou
D7. Ce raccordement laisse `D0 / A0` entièrement disponible pour le WSP2110.

## Grove HCHO WSP2110 — entrée analogique protégée

Le WSP2110 n’est pas raccordé au hub I²C. Alimenter son module Grove en 5 V et
relier toutes les masses. Sa sortie analogique peut dépasser 3,3 V : ne jamais
la brancher directement sur `D0` du XIAO.

Réaliser ce pont diviseur :

```text
Sortie SIG du WSP2110 ── 10 kΩ ──┬── D0 du XIAO
                                  │
                                 20 kΩ
                                  │
                                 GND
```

Ajouter de préférence un condensateur céramique de 100 nF entre `D0` et GND,
au plus près du XIAO. Le rapport du pont est alors 2/3, valeur déjà définie par
`EASE_WSP2110_DIVIDER_RATIO`. Vérifier au multimètre que D0 reste toujours sous
3,3 V avant de le connecter à la carte.

Le firmware accepte que le WSP2110 ne soit pas installé : D0 peut rester non
connecté, le démarrage continue normalement et la valeur HCHO reste indiquée
comme indisponible jusqu’à la première lecture valide. Un bref test logiciel
compare les résistances de tirage vers la masse et vers le 3,3 V afin d’empêcher
une entrée D0 flottante d’être prise par erreur pour un capteur présent. Trois
lectures valides consécutives sont ensuite nécessaires pour publier le HCHO.

## Module microSD Adafruit 254

| Adafruit 254 | XIAO ESP32S3 | Fonction |
|---|---|---|
| `5V` | `5V/VBUS` | alimentation du régulateur du module |
| `GND` | `GND` | masse commune |
| `CLK` | `D8 / GPIO7` | horloge SPI |
| `DO` | `D9 / GPIO8` | MISO |
| `DI` | `D10 / GPIO9` | MOSI |
| `CS` | `D2 / GPIO3` | sélection de la carte |

Le raccordement `5V/VBUS` suppose une alimentation USB 5 V. Pour une station
alimentée uniquement par batterie, vérifier que le rail 5 V reste disponible
ou prévoir une alimentation adaptée pour le module microSD et les capteurs.

## Alimentation

Le HM3301 et le SCD30 contiennent des éléments actifs et le HM3301 possède un
ventilateur. Employer une alimentation USB 5 V stable, avec une marge de
courant suffisante. Toutes les masses doivent être communes.

Ne jamais appliquer un signal ou une tension analogique 5 V directement aux GPIO de la XIAO.
L’Adafruit 254 réalise l’adaptation nécessaire pour la microSD.

## Implantation des capteurs

- éloigner le DHT20 de la chaleur produite par la XIAO et le régulateur ;
- ne pas placer les entrées d’air du SCD30, du HM3301 et du WSP2110 contre une paroi ;
- éviter que la sortie d’air du HM3301 souffle directement sur le DHT20 ;
- laisser le SGP40 exposé au même volume d’air que les autres capteurs ;
- éloigner le WSP2110 des vapeurs de solvants et garder son module sous tension
  au moins 120 heures avant l’étalonnage initial ;
- conserver le buzzer et l’OLED hors du trajet principal de l’air.
