#pragma once

// Copiez ce fichier sous le nom config.h pour personnaliser la station.
// Si config.h est absent, le firmware utilise ces valeurs et crée le point
// d'accès Wi-Fi local EASE-XIAO.

#define EASE_WIFI_SSID ""
#define EASE_WIFI_PASSWORD ""
#define EASE_HOSTNAME "ease-xiao"
#define EASE_AP_PASSWORD "ease-air"

// Grove 8-Channel I2C Hub TCA9548A. Tous les capteurs du projet ont des
// adresses différentes : les huit canaux peuvent donc rester ouverts.
#define EASE_I2C_MUX_ENABLED 1
#define EASE_I2C_MUX_ADDRESS 0x70
#define EASE_I2C_MUX_CHANNEL_MASK 0xFF

// Fuseau Europe/Paris avec changements automatiques heure d'été/hiver.
#define EASE_TIMEZONE "CET-1CEST,M3.5.0/2,M10.5.0/3"
#define EASE_NTP_SERVER_1 "pool.ntp.org"
#define EASE_NTP_SERVER_2 "time.cloudflare.com"

// Broches XIAO ESP32S3.
#define EASE_BUZZER_PIN D1
#define EASE_OLED_BUTTON_PIN D7
#define EASE_VENTILATION_BUTTON_PIN D0

// Laisser à 0 tant que le WSP2110 et son pont diviseur ne sont pas installés.
// Passer à 1 uniquement après avoir réalisé et vérifié le câblage décrit dans
// CABLAGE.md.
#define EASE_WSP2110_ENABLED 0
#define EASE_WSP2110_PIN D0
#define EASE_SD_CS_PIN D2
#define EASE_SD_SCK_PIN D8
#define EASE_SD_MISO_PIN D9
#define EASE_SD_MOSI_PIN D10

// Périodes de fonctionnement.
#define EASE_SAMPLE_INTERVAL_MS 10000UL
#define EASE_SGP40_INTERVAL_MS 1000UL
#define EASE_OLED_TIMEOUT_MS 20000UL
#define EASE_BUTTON_DEBOUNCE_MS 35UL
#define EASE_SD_FLUSH_INTERVAL_MS 60000UL

// Seuils de l'indice intérieur EASE. Ils restent modifiables ici sans toucher
// au reste du firmware.
#define EASE_CO2_YELLOW_PPM 800.0f
#define EASE_CO2_ORANGE_PPM 1000.0f
#define EASE_CO2_RED_PPM 1500.0f
#define EASE_CO2_RESET_PPM 1400.0f

#define EASE_VOC_YELLOW_INDEX 120.0f
#define EASE_VOC_ORANGE_INDEX 180.0f
#define EASE_VOC_RED_INDEX 220.0f
#define EASE_VOC_RESET_INDEX 200.0f

// WSP2110 : estimation indicative en ppm d'après la courbe typique Seeed.
// Le module doit être étalonné individuellement après au moins 120 h de chauffe.
#define EASE_WSP2110_R0_RATIO 34.28f
#define EASE_WSP2110_VC_VOLTS 5.0f
// Avec le pont conseillé 10 kΩ (haut) + 20 kΩ (bas), Vadc/Vsortie = 2/3.
#define EASE_WSP2110_DIVIDER_RATIO 0.6666667f

// Seuils pédagogiques configurables, dans la plage indicative du WSP2110.
// Ils ne correspondent pas à des seuils réglementaires certifiés.
#define EASE_HCHO_YELLOW_PPM 1.0f
#define EASE_HCHO_ORANGE_PPM 2.0f
#define EASE_HCHO_RED_PPM 5.0f
#define EASE_HCHO_RESET_PPM 4.5f

#define EASE_PM25_YELLOW_UGM3 15.0f
#define EASE_PM25_ORANGE_UGM3 25.0f
#define EASE_PM25_RED_UGM3 50.0f
#define EASE_PM25_RESET_UGM3 45.0f

#define EASE_PM10_YELLOW_UGM3 45.0f
#define EASE_PM10_ORANGE_UGM3 50.0f
#define EASE_PM10_RED_UGM3 100.0f
#define EASE_PM10_RESET_UGM3 90.0f

// Seuil distinct pour la moyenne glissante PM2.5 sur 24 heures.
#define EASE_PM25_24H_LIMIT_UGM3 15.0f
#define EASE_PM25_24H_RESET_UGM3 13.5f

// Une alerte doit être confirmée par ce nombre de mesures consécutives.
#define EASE_CONFIRMATION_SAMPLES 3
