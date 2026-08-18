# Transcription nettoyée — EASE Air Quality Lab

## Introduction

Bonjour. Je vais vous présenter notre **EASE Air Quality Lab**.

L’objectif est de réaliser une station environnementale permettant de mesurer la qualité de l’air, aussi bien en intérieur qu’en extérieur.

Dans la phase actuelle, nous disposons d’un boîtier servant de banc de test multicapteur. Il permet de comparer différentes technologies afin de sélectionner les capteurs les mieux adaptés aux futurs usages.

Le système est modulaire. Nous utilisons des cartes **Arduino UNO R4 WiFi** avec l’écosystème **Grove**, ce qui évite d’avoir à réaliser des soudures.

Deux architectures ont été développées :

- une version utilisant le réseau Wi-Fi local ;
- une version longue portée utilisant la technologie radio LoRa.

## Alimentation et interface utilisateur

Le boîtier est conçu pour être flexible. Il peut être alimenté par un chargeur USB externe ou par une batterie placée directement à l’intérieur.

Un petit écran OLED permet de consulter les données en temps réel sur le boîtier et de vérifier que le système fonctionne correctement. Un bouton permet d’éteindre cet écran afin d’économiser la batterie.

## Première architecture : version Wi-Fi

La version Wi-Fi utilise une seule carte Arduino. Celle-ci se connecte au réseau Wi-Fi local et transmet les données au serveur informatique, développé en Python, au moyen de requêtes HTTP POST.

Cette architecture est simple et directe. Son principal inconvénient est sa portée, limitée à la couverture du réseau Wi-Fi. La configuration peut également être difficile dans les lycées, notamment en raison des changements d’adresse IP. Il faudra donc prévoir l’utilisation d’adresses IP fixes.

## Deuxième architecture : version LoRa

La version LoRa utilise deux cartes Arduino :

- un émetteur alimenté par batterie, équipé des capteurs et d’une antenne LoRa ;
- un récepteur connecté en USB à l’ordinateur sur lequel fonctionne le serveur Python.

La transmission entre l’émetteur et le récepteur s’effectue par ondes radio. Dans de bonnes conditions, la portée peut atteindre plusieurs kilomètres. Le récepteur transmet ensuite les données à l’ordinateur par sa connexion USB.

L’intérêt de cette architecture est de pouvoir installer les capteurs dans de nombreux endroits — un jardin, une forêt ou un autre bâtiment — sans dépendre d’un réseau Wi-Fi. Son inconvénient est qu’elle nécessite deux cartes Arduino.

## Récupération et traitement des données

Un serveur informatique développé en Python collecte l’ensemble des données. Il repose sur le framework **Flask** et utilise **Waitress** comme serveur de production.

Dans la version Wi-Fi, le serveur écoute sur le **port 5000** les paquets HTTP envoyés par l’Arduino.

Dans la version LoRa, le serveur écoute le port COM du récepteur USB au moyen de la bibliothèque **PySerial**.

## Tableau de bord

Une fois les données récupérées, elles sont présentées dans un tableau de bord accessible depuis une page web.

Ce tableau de bord affiche :

- les valeurs en temps réel ;
- les graphiques en temps réel lorsque le système est connecté ;
- de la documentation ;
- des informations sur le montage.

## Sauvegarde des données

Les données sont enregistrées dans un fichier CSV exploitable avec un tableur. Ce fichier comporte 23 colonnes et reçoit une nouvelle ligne toutes les dix secondes. Cet intervalle d’enregistrement est paramétrable.

Les informations collectées comprennent notamment :

- l’horodatage, avec la date et l’heure ;
- les paramètres ambiants : température, humidité et pression ;
- les mesures de qualité de l’air : CO₂, COV, eCO₂ et HCHO ;
- les mesures de particules fines.

Une horloge RTC intégrée à la carte Arduino garantit un horodatage fiable.

Les données sont également stockées sur une carte SD située dans le boîtier des capteurs. Cette sauvegarde locale apporte une sécurité supplémentaire en cas de problème de liaison.

Le fichier est transféré automatiquement vers le serveur. Lorsque la transmission fonctionne correctement, le même fichier CSV est donc disponible sur l’ordinateur, ce qui évite d’avoir à retirer la carte SD du boîtier.

## Capteurs

Le banc de test rassemble de nombreux capteurs différents :

- des capteurs d’environnement général pour la température et l’humidité ;
- des capteurs mesurant les composés organiques volatils, ou COV ;
- un capteur optique mesurant le CO₂ réel ;
- des capteurs de microparticules ;
- des capteurs mesurant notamment le NO₂, le CO, l’éthanol et les COV ;
- un capteur de formaldéhyde, ou HCHO ;
- des capteurs mesurant les TVOC ;
- un capteur fournissant un indice COV.

L’objectif est de comparer ces technologies afin de déterminer quels capteurs devront être retenus dans les versions finales des stations, selon les usages visés.

## Évolutions prévues

Une ventilation active a été intégrée, car un possible effet de serre a été observé à l’intérieur des boîtiers. Des essais devront déterminer si l’utilisation d’un ventilateur est réellement nécessaire.

Un module GPS pourra également être ajouté afin d’obtenir une géolocalisation précise sans avoir à saisir manuellement différentes informations.

La solution finale devra être adaptée à chaque usage. Plusieurs choix restent à effectuer :

- alimentation par batterie ;
- alimentation sur secteur ;
- fonctionnement autonome avec un panneau solaire ;
- communication par Wi-Fi ;
- communication par LoRa ;
- liaison USB directe.

## Trois utilisations finales

### 1. Mesure de la qualité de l’air dans une salle de classe

Cette station comportera :

- un capteur de CO₂ ;
- un capteur de COV ;
- des capteurs de température et d’humidité ;
- un bouton-poussoir permettant de signaler une action de ventilation, par exemple l’ouverture d’une porte ou d’une fenêtre ;
- une visualisation des données sur le serveur.

### 2. Mesure extérieure sur le trajet domicile-école des élèves

Cette utilisation nécessitera un boîtier compact fonctionnant sur batterie. Il permettra notamment de mesurer les particules fines ainsi qu’un autre gaz, qui reste à préciser.

### 3. Mesure extérieure dans un lieu spécifique

La troisième station servira à mesurer la qualité de l’air dans un emplacement déterminé, par exemple sur le toit ou dans la cour d’un lycée.

Le boîtier devra être autonome en énergie, transmettre les données au moyen d’un module LoRa et résister aux intempéries.

Merci de votre attention.
