#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <U8x8lib.h>
#include <Adafruit_SGP40.h>
#include <SparkFun_SCD30_Arduino_Library.h>
#include <DHT20.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>

#include "dashboard.h"
#if __has_include("config.h")
#include "config.h"
#else
#include "config.example.h"
#endif

#if EASE_WSP2110_ENABLED
static_assert(EASE_VENTILATION_BUTTON_PIN != EASE_WSP2110_PIN,
              "Le bouton de ventilation et le WSP2110 ne peuvent pas partager la meme broche");
#endif

// -----------------------------------------------------------------------------
// EASE XIAO ESP32S3 — station autonome de qualité de l'air
// -----------------------------------------------------------------------------

constexpr uint8_t RTC_ADDRESS = 0x51;
constexpr uint8_t HM3301_ADDRESS = 0x40;
constexpr size_t HISTORY_CAPACITY = 8640;  // 24 h à une mesure toutes les 10 s
constexpr size_t EVENT_CAPACITY = 200;
#if EASE_WSP2110_ENABLED
constexpr uint8_t SENSOR_TOTAL = 5;
#else
constexpr uint8_t SENSOR_TOTAL = 4;
#endif

U8X8_SSD1306_128X64_NONAME_HW_I2C oled(U8X8_PIN_NONE);
Adafruit_SGP40 sgp40;
SCD30 scd30;
DHT20 dht20;
WebServer server(80);
SPIClass sdSpi(FSPI);

struct Measurement {
    uint32_t timestamp;
    float co2;
    float voc;
    float hcho;
    float pm1;
    float pm25;
    float pm10;
    float temperature;
    float humidity;
    uint8_t quality;
    bool ventilation;
};

struct EventRecord {
    uint32_t start;
    uint32_t end;
    uint32_t peakAt;
    char parameter[14];
    char unit[12];
    float threshold;
    float average;
    float maximum;
};

struct EventTracker {
    bool active = false;
    uint8_t aboveCount = 0;
    uint8_t belowCount = 0;
    uint32_t pendingStart = 0;
    uint32_t start = 0;
    uint32_t peakAt = 0;
    float pendingSum = 0;
    float pendingMax = -INFINITY;
    float sum = 0;
    float maximum = -INFINITY;
    uint32_t samples = 0;
};

struct EventRule {
    const char* parameter;
    const char* unit;
    float threshold;
    float reset;
};

enum EventId : uint8_t {
    EVENT_CO2,
    EVENT_VOC,
    EVENT_HCHO,
    EVENT_PM25,
    EVENT_PM25_24H,
    EVENT_PM10,
    EVENT_RULE_COUNT
};

const EventRule EVENT_RULES[EVENT_RULE_COUNT] = {
    {"CO2", "ppm", EASE_CO2_RED_PPM, EASE_CO2_RESET_PPM},
    {"VOC", "index", EASE_VOC_RED_INDEX, EASE_VOC_RESET_INDEX},
    {"HCHO estime", "ppm", EASE_HCHO_RED_PPM, EASE_HCHO_RESET_PPM},
    {"PM2.5", "ug/m3", EASE_PM25_RED_UGM3, EASE_PM25_RESET_UGM3},
    {"PM2.5 24h", "ug/m3", EASE_PM25_24H_LIMIT_UGM3, EASE_PM25_24H_RESET_UGM3},
    {"PM10", "ug/m3", EASE_PM10_RED_UGM3, EASE_PM10_RESET_UGM3},
};

Measurement currentMeasurement = {0, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, 0, false};
Measurement* historyBuffer = nullptr;
size_t historyHead = 0;
size_t historyCount = 0;
double pm25Sum = 0;
uint32_t pm25ValidCount = 0;

EventRecord* eventBuffer = nullptr;
size_t eventHead = 0;
size_t eventCount = 0;
EventTracker eventTrackers[EVENT_RULE_COUNT];

bool rtcOk = false;
bool i2cMuxOk = false;
bool sdOk = false;
bool scdOk = false;
bool hmOk = false;
bool sgpOk = false;
bool wspOk = false;
bool dhtOk = false;
bool apMode = false;
bool alarmActive = false;
uint8_t globalRedCount = 0;
const char* qualityCause = "Aucun";

File measurementFile;
String measurementPath;
uint32_t lastMeasurementFlush = 0;
uint8_t hmBuffer[29];
uint8_t hmConsecutiveFailures = 0;
size_t hmLastReceivedBytes = 0;
bool hmHasValidFrame = false;
float wspRsRatio = NAN;
float wspAdcVoltage = NAN;
uint8_t oledPage = 0;
bool oledAwake = true;
bool buttonStableState = false;
bool buttonLastReading = false;
bool ventilationActive = false;
bool ventilationButtonStableState = false;
bool ventilationButtonLastReading = false;
uint32_t lastSample = 0;
uint32_t lastSgp40 = 0;
uint32_t lastButtonChange = 0;
uint32_t lastVentilationButtonChange = 0;
uint32_t lastOledActivity = 0;
uint32_t lastWifiAttempt = 0;
uint32_t lastRtcNtpWrite = 0;
uint32_t lastBuzzerToggle = 0;
bool buzzerOn = false;

// -----------------------------------------------------------------------------
// Utilitaires
// -----------------------------------------------------------------------------

bool initializeI2cMultiplexer() {
#if EASE_I2C_MUX_ENABLED
    Wire.beginTransmission(EASE_I2C_MUX_ADDRESS);
    Wire.write(EASE_I2C_MUX_CHANNEL_MASK);
    bool ok = Wire.endTransmission() == 0;
    Serial.printf("Hub I2C TCA9548A 0x%02X: %s (canaux 0x%02X)\n",
                  EASE_I2C_MUX_ADDRESS, ok ? "OK" : "ERREUR",
                  EASE_I2C_MUX_CHANNEL_MASK);
    return ok;
#else
    Serial.println("Multiplexeur I2C desactive");
    return true;
#endif
}

uint8_t bcdToDec(uint8_t value) {
    return (value / 16 * 10) + (value % 16);
}

uint8_t decToBcd(uint8_t value) {
    return (value / 10 * 16) + (value % 10);
}

String jsonNumber(float value, uint8_t decimals = 1) {
    return isfinite(value) ? String(value, static_cast<unsigned int>(decimals)) : String("null");
}

String localIso(uint32_t epoch) {
    if (epoch < 946684800UL) return String("heure inconnue");
    time_t raw = static_cast<time_t>(epoch);
    struct tm tmValue;
    localtime_r(&raw, &tmValue);
    char buffer[24];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tmValue);
    return String(buffer);
}

String dayPath(uint32_t epoch) {
    time_t raw = static_cast<time_t>(epoch);
    struct tm tmValue;
    localtime_r(&raw, &tmValue);
    char buffer[28];
    strftime(buffer, sizeof(buffer), "/data/%Y%m%d.csv", &tmValue);
    return String(buffer);
}

uint32_t nowEpoch() {
    time_t value = time(nullptr);
    return value > 0 ? static_cast<uint32_t>(value) : 0;
}

// -----------------------------------------------------------------------------
// RTC Grove PCF85063
// -----------------------------------------------------------------------------

bool readRtc(struct tm& value) {
    Wire.beginTransmission(RTC_ADDRESS);
    Wire.write(0x04);
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom(RTC_ADDRESS, static_cast<uint8_t>(7)) != 7) return false;

    uint8_t secondsRaw = Wire.read();
    value = {};
    value.tm_sec = bcdToDec(secondsRaw & 0x7F);
    value.tm_min = bcdToDec(Wire.read() & 0x7F);
    value.tm_hour = bcdToDec(Wire.read() & 0x3F);
    value.tm_mday = bcdToDec(Wire.read() & 0x3F);
    value.tm_wday = bcdToDec(Wire.read() & 0x07);
    value.tm_mon = bcdToDec(Wire.read() & 0x1F) - 1;
    value.tm_year = bcdToDec(Wire.read()) + 100;
    value.tm_isdst = -1;

    bool validFields = value.tm_year >= 120 && value.tm_year <= 199 &&
                       value.tm_mon >= 0 && value.tm_mon <= 11 &&
                       value.tm_mday >= 1 && value.tm_mday <= 31 &&
                       value.tm_hour <= 23 && value.tm_min <= 59 && value.tm_sec <= 59;
    return !(secondsRaw & 0x80) && validFields;
}

bool writeRtc(time_t epoch) {
    struct tm localValue;
    localtime_r(&epoch, &localValue);

    Wire.beginTransmission(RTC_ADDRESS);
    Wire.write(0x00);
    Wire.write(0x00);
    Wire.write(0x00);
    if (Wire.endTransmission() != 0) return false;

    Wire.beginTransmission(RTC_ADDRESS);
    Wire.write(0x04);
    Wire.write(decToBcd(localValue.tm_sec));
    Wire.write(decToBcd(localValue.tm_min));
    Wire.write(decToBcd(localValue.tm_hour));
    Wire.write(decToBcd(localValue.tm_mday));
    Wire.write(decToBcd(localValue.tm_wday));
    Wire.write(decToBcd(localValue.tm_mon + 1));
    Wire.write(decToBcd((localValue.tm_year + 1900) - 2000));
    return Wire.endTransmission() == 0;
}

void initializeClockFromRtc() {
    setenv("TZ", EASE_TIMEZONE, 1);
    tzset();
    struct tm rtcTime;
    rtcOk = readRtc(rtcTime);
    if (!rtcOk) {
        Serial.println("RTC PCF85063 absente ou heure invalide");
        return;
    }
    time_t epoch = mktime(&rtcTime);
    timeval tv = {epoch, 0};
    settimeofday(&tv, nullptr);
    Serial.printf("Heure RTC chargee: %s\n", localIso(epoch).c_str());
}

void synchronizeNtp() {
    if (WiFi.status() != WL_CONNECTED || apMode) return;
    configTzTime(EASE_TIMEZONE, EASE_NTP_SERVER_1, EASE_NTP_SERVER_2);
    struct tm networkTime;
    if (getLocalTime(&networkTime, 5000)) {
        time_t epoch = time(nullptr);
        rtcOk = writeRtc(epoch);
        lastRtcNtpWrite = millis();
        Serial.printf("RTC corrigee par NTP: %s\n", localIso(epoch).c_str());
    } else {
        Serial.println("NTP indisponible, conservation de l'heure RTC");
    }
}

// -----------------------------------------------------------------------------
// Historique en PSRAM
// -----------------------------------------------------------------------------

Measurement& historyAt(size_t logicalIndex) {
    size_t first = (historyHead + HISTORY_CAPACITY - historyCount) % HISTORY_CAPACITY;
    return historyBuffer[(first + logicalIndex) % HISTORY_CAPACITY];
}

void removeOldestHistory() {
    if (!historyCount) return;
    Measurement& oldest = historyAt(0);
    if (isfinite(oldest.pm25)) {
        pm25Sum -= oldest.pm25;
        if (pm25ValidCount) pm25ValidCount--;
    }
    historyCount--;
}

void appendHistory(const Measurement& measurement) {
    if (!historyBuffer) return;
    uint32_t cutoff = measurement.timestamp > 86400UL ? measurement.timestamp - 86400UL : 0;
    while (historyCount && historyAt(0).timestamp < cutoff) removeOldestHistory();
    if (historyCount == HISTORY_CAPACITY) removeOldestHistory();

    historyBuffer[historyHead] = measurement;
    historyHead = (historyHead + 1) % HISTORY_CAPACITY;
    historyCount++;
    if (isfinite(measurement.pm25)) {
        pm25Sum += measurement.pm25;
        pm25ValidCount++;
    }
}

float pm25Average24h() {
    return pm25ValidCount ? static_cast<float>(pm25Sum / pm25ValidCount) : NAN;
}

uint32_t historyCoverageSeconds() {
    if (historyCount < 2) return 0;
    return historyAt(historyCount - 1).timestamp - historyAt(0).timestamp;
}

bool pm25AverageReady() {
    return historyCoverageSeconds() >= 23UL * 3600UL;
}

// -----------------------------------------------------------------------------
// Stockage microSD
// -----------------------------------------------------------------------------

bool parseMeasurementLine(const char* line, Measurement& m) {
    unsigned long timestamp = 0;
    int quality = 0;
    int ventilation = 0;
    int fields = sscanf(line, "%lu,%f,%f,%f,%f,%f,%f,%f,%f,%d,%d",
                        &timestamp, &m.co2, &m.voc, &m.hcho, &m.pm1, &m.pm25,
                        &m.pm10, &m.temperature, &m.humidity, &quality, &ventilation);
    m.timestamp = static_cast<uint32_t>(timestamp);
    m.quality = static_cast<uint8_t>(quality);
    m.ventilation = fields >= 11 && ventilation != 0;
    return (fields == 10 || fields == 11) && m.timestamp > 946684800UL;
}

void loadHistoryFile(const String& path, uint32_t cutoff) {
    File file = SD.open(path, FILE_READ);
    if (!file) return;
    char line[220];
    while (file.available()) {
        size_t count = file.readBytesUntil('\n', line, sizeof(line) - 1);
        line[count] = '\0';
        if (line[0] < '0' || line[0] > '9') continue;
        Measurement m;
        if (parseMeasurementLine(line, m) && m.timestamp >= cutoff) appendHistory(m);
        delay(0);
    }
    file.close();
}

void loadRecentHistory() {
    uint32_t now = nowEpoch();
    if (!sdOk || now < 946684800UL) return;
    uint32_t cutoff = now - 86400UL;
    String yesterday = dayPath(cutoff);
    String today = dayPath(now);
    loadHistoryFile(yesterday, cutoff);
    if (today != yesterday) loadHistoryFile(today, cutoff);
    Serial.printf("Historique restaure: %u mesures\n", static_cast<unsigned>(historyCount));
}

void ensureMeasurementFile(uint32_t timestamp) {
    if (!sdOk) return;
    String wantedPath = dayPath(timestamp);
    if (measurementFile && wantedPath == measurementPath) return;
    if (measurementFile) {
        measurementFile.flush();
        measurementFile.close();
    }
    bool isNew = !SD.exists(wantedPath);
    measurementFile = SD.open(wantedPath, FILE_APPEND);
    measurementPath = wantedPath;
    if (measurementFile && isNew) {
        measurementFile.println("epoch,co2_ppm,voc_index,hcho_estime_ppm,pm1_ugm3,pm25_ugm3,pm10_ugm3,temperature_c,humidity_pct,quality,ventilation");
        measurementFile.flush();
    }
}

void logMeasurement(const Measurement& m) {
    if (!sdOk || m.timestamp < 946684800UL) return;
    ensureMeasurementFile(m.timestamp);
    if (!measurementFile) {
        sdOk = false;
        return;
    }
    measurementFile.printf("%lu,%.1f,%.0f,%.1f,%.1f,%.1f,%.1f,%.2f,%.2f,%u,%u\n",
                           static_cast<unsigned long>(m.timestamp), m.co2, m.voc,
                           m.hcho, m.pm1, m.pm25, m.pm10, m.temperature,
                           m.humidity, m.quality, static_cast<unsigned>(m.ventilation));
    if (millis() - lastMeasurementFlush >= EASE_SD_FLUSH_INTERVAL_MS) {
        measurementFile.flush();
        lastMeasurementFlush = millis();
    }
}

void appendEventMemory(const EventRecord& event) {
    if (!eventBuffer) return;
    eventBuffer[eventHead] = event;
    eventHead = (eventHead + 1) % EVENT_CAPACITY;
    if (eventCount < EVENT_CAPACITY) eventCount++;
}

EventRecord& eventAt(size_t logicalIndex) {
    size_t first = (eventHead + EVENT_CAPACITY - eventCount) % EVENT_CAPACITY;
    return eventBuffer[(first + logicalIndex) % EVENT_CAPACITY];
}

void persistEvent(const EventRecord& event) {
    appendEventMemory(event);
    if (!sdOk) return;
    bool isNew = !SD.exists("/events.csv");
    File file = SD.open("/events.csv", FILE_APPEND);
    if (!file) return;
    if (isNew) file.println("start_epoch,end_epoch,parameter,unit,threshold,average,maximum,peak_epoch");
    file.printf("%lu,%lu,%s,%s,%.2f,%.2f,%.2f,%lu\n",
                static_cast<unsigned long>(event.start),
                static_cast<unsigned long>(event.end), event.parameter, event.unit,
                event.threshold, event.average, event.maximum,
                static_cast<unsigned long>(event.peakAt));
    file.flush();
    file.close();
}

void loadEvents() {
    if (!sdOk || !SD.exists("/events.csv")) return;
    File file = SD.open("/events.csv", FILE_READ);
    char line[180];
    while (file.available()) {
        size_t length = file.readBytesUntil('\n', line, sizeof(line) - 1);
        line[length] = '\0';
        if (line[0] < '0' || line[0] > '9') continue;
        EventRecord event = {};
        unsigned long start, end, peak;
        int fields = sscanf(line, "%lu,%lu,%13[^,],%11[^,],%f,%f,%f,%lu",
                            &start, &end, event.parameter, event.unit,
                            &event.threshold, &event.average, &event.maximum, &peak);
        if (fields == 8) {
            event.start = start;
            event.end = end;
            event.peakAt = peak;
            appendEventMemory(event);
        }
    }
    file.close();
}

void initializeStorage() {
    sdSpi.begin(EASE_SD_SCK_PIN, EASE_SD_MISO_PIN, EASE_SD_MOSI_PIN, EASE_SD_CS_PIN);
    sdOk = SD.begin(EASE_SD_CS_PIN, sdSpi, 10000000);
    if (!sdOk) {
        Serial.println("microSD indisponible: fonctionnement temporaire en PSRAM");
        return;
    }
    if (!SD.exists("/data")) SD.mkdir("/data");
    loadRecentHistory();
    loadEvents();
}

// -----------------------------------------------------------------------------
// Qualité de l'air et événements
// -----------------------------------------------------------------------------

uint8_t pollutantLevel(float value, float yellow, float orange, float red) {
    if (!isfinite(value)) return 0;
    if (value >= red) return 3;
    if (value >= orange) return 2;
    if (value >= yellow) return 1;
    return 0;
}

uint8_t computeQuality() {
    uint8_t best = 0;
    qualityCause = "Aucun";
    struct Candidate { uint8_t level; const char* name; } candidates[] = {
        {pollutantLevel(currentMeasurement.co2, EASE_CO2_YELLOW_PPM, EASE_CO2_ORANGE_PPM, EASE_CO2_RED_PPM), "CO2"},
        {pollutantLevel(currentMeasurement.voc, EASE_VOC_YELLOW_INDEX, EASE_VOC_ORANGE_INDEX, EASE_VOC_RED_INDEX), "VOC"},
#if EASE_WSP2110_ENABLED
        {pollutantLevel(currentMeasurement.hcho, EASE_HCHO_YELLOW_PPM, EASE_HCHO_ORANGE_PPM, EASE_HCHO_RED_PPM), "HCHO estime"},
#endif
        {pollutantLevel(currentMeasurement.pm25, EASE_PM25_YELLOW_UGM3, EASE_PM25_ORANGE_UGM3, EASE_PM25_RED_UGM3), "PM2.5"},
        {pollutantLevel(currentMeasurement.pm10, EASE_PM10_YELLOW_UGM3, EASE_PM10_ORANGE_UGM3, EASE_PM10_RED_UGM3), "PM10"},
    };
    for (const Candidate& candidate : candidates) {
        if (candidate.level > best) {
            best = candidate.level;
            qualityCause = candidate.name;
        }
    }
    return best;
}

void closeEvent(EventId id, uint32_t endTime) {
    EventTracker& tracker = eventTrackers[id];
    const EventRule& rule = EVENT_RULES[id];
    if (!tracker.active) return;
    EventRecord event = {};
    event.start = tracker.start;
    event.end = max(endTime, tracker.start);
    event.peakAt = tracker.peakAt;
    strlcpy(event.parameter, rule.parameter, sizeof(event.parameter));
    strlcpy(event.unit, rule.unit, sizeof(event.unit));
    event.threshold = rule.threshold;
    event.average = tracker.samples ? tracker.sum / tracker.samples : tracker.maximum;
    event.maximum = tracker.maximum;
    persistEvent(event);
    tracker = EventTracker();
}

void updateEvent(EventId id, float value, uint32_t timestamp, bool enabled = true) {
    EventTracker& tracker = eventTrackers[id];
    const EventRule& rule = EVENT_RULES[id];
    if (!enabled || !isfinite(value)) return;

    if (!tracker.active) {
        if (value >= rule.threshold) {
            if (tracker.aboveCount == 0) {
                tracker.pendingStart = timestamp;
                tracker.pendingSum = 0;
                tracker.pendingMax = value;
            }
            tracker.aboveCount++;
            tracker.pendingSum += value;
            tracker.pendingMax = max(tracker.pendingMax, value);
            if (tracker.aboveCount >= EASE_CONFIRMATION_SAMPLES) {
                tracker.active = true;
                tracker.start = tracker.pendingStart;
                tracker.sum = tracker.pendingSum;
                tracker.samples = tracker.aboveCount;
                tracker.maximum = tracker.pendingMax;
                tracker.peakAt = timestamp;
                tracker.belowCount = 0;
            }
        } else {
            tracker.aboveCount = 0;
            tracker.pendingSum = 0;
            tracker.pendingMax = -INFINITY;
        }
        return;
    }

    if (value < rule.reset) {
        tracker.belowCount++;
        if (tracker.belowCount >= EASE_CONFIRMATION_SAMPLES) {
            uint32_t endTime = timestamp - (EASE_CONFIRMATION_SAMPLES - 1) * (EASE_SAMPLE_INTERVAL_MS / 1000UL);
            closeEvent(id, endTime);
        }
    } else {
        tracker.belowCount = 0;
        tracker.sum += value;
        tracker.samples++;
        if (value > tracker.maximum) {
            tracker.maximum = value;
            tracker.peakAt = timestamp;
        }
    }
}

void updateAllEvents(uint32_t timestamp) {
    updateEvent(EVENT_CO2, currentMeasurement.co2, timestamp, scdOk);
    updateEvent(EVENT_VOC, currentMeasurement.voc, timestamp, sgpOk);
#if EASE_WSP2110_ENABLED
    updateEvent(EVENT_HCHO, currentMeasurement.hcho, timestamp, wspOk);
#endif
    updateEvent(EVENT_PM25, currentMeasurement.pm25, timestamp, hmOk);
    updateEvent(EVENT_PM25_24H, pm25Average24h(), timestamp, pm25AverageReady());
    updateEvent(EVENT_PM10, currentMeasurement.pm10, timestamp, hmOk);
}

void updateAlarm() {
    if (currentMeasurement.quality == 3) {
        if (globalRedCount < EASE_CONFIRMATION_SAMPLES) globalRedCount++;
    } else if (currentMeasurement.quality <= 2) {
        globalRedCount = 0;
    }
    alarmActive = globalRedCount >= EASE_CONFIRMATION_SAMPLES;
}

void serviceBuzzer() {
    if (!alarmActive) {
        if (buzzerOn) noTone(EASE_BUZZER_PIN);
        buzzerOn = false;
        return;
    }
    uint32_t interval = buzzerOn ? 450UL : 1200UL;
    if (millis() - lastBuzzerToggle >= interval) {
        lastBuzzerToggle = millis();
        buzzerOn = !buzzerOn;
        if (buzzerOn) tone(EASE_BUZZER_PIN, 2700);
        else noTone(EASE_BUZZER_PIN);
    }
}

// -----------------------------------------------------------------------------
// Capteurs
// -----------------------------------------------------------------------------

bool validHmChecksum(const uint8_t* data) {
    uint8_t sum = 0;
    for (uint8_t i = 0; i < 28; i++) sum += data[i];
    return sum == data[28];
}

bool initializeHm3301() {
#if EASE_I2C_MUX_ENABLED
    Wire.beginTransmission(EASE_I2C_MUX_ADDRESS);
    Wire.write(EASE_I2C_MUX_CHANNEL_MASK);
    if (Wire.endTransmission() != 0) return false;
#endif
    Wire.beginTransmission(HM3301_ADDRESS);
    Wire.write(0x88);  // sélection de la communication I²C
    return Wire.endTransmission() == 0;
}

bool readHm3301(uint8_t* data, size_t length) {
    for (uint8_t attempt = 0; attempt < 3; attempt++) {
        if (attempt > 0) {
            initializeHm3301();
            delay(25);
        }

        while (Wire.available()) Wire.read();
        hmLastReceivedBytes = Wire.requestFrom(
            static_cast<uint8_t>(HM3301_ADDRESS),
            static_cast<uint8_t>(length), true);
        if (hmLastReceivedBytes == length) {
            for (size_t i = 0; i < length; i++) data[i] = Wire.read();
            if (validHmChecksum(data)) return true;
        } else {
            while (Wire.available()) Wire.read();
        }
        delay(25);
    }
    return false;
}

float readWsp2110() {
    constexpr uint8_t SAMPLE_COUNT = 16;
    uint32_t millivoltSum = 0;
    for (uint8_t i = 0; i < SAMPLE_COUNT; i++) {
        millivoltSum += analogReadMilliVolts(EASE_WSP2110_PIN);
        delay(2);
    }

    wspAdcVoltage = (millivoltSum / static_cast<float>(SAMPLE_COUNT)) / 1000.0f;
    const float sensorVoltage = wspAdcVoltage / EASE_WSP2110_DIVIDER_RATIO;
    if (sensorVoltage <= 0.01f || sensorVoltage >= EASE_WSP2110_VC_VOLTS * 0.995f) {
        wspRsRatio = NAN;
        wspOk = false;
        return NAN;
    }

    // Courbe indicative du module Grove WSP2110 : Rs/R0 -> ppm.
    wspRsRatio = EASE_WSP2110_VC_VOLTS / sensorVoltage - 1.0f;
    if (!isfinite(wspRsRatio) || wspRsRatio <= 0.0f || EASE_WSP2110_R0_RATIO <= 0.0f) {
        wspOk = false;
        return NAN;
    }
    const float ppm = powf(10.0f,
        (log10f(wspRsRatio / EASE_WSP2110_R0_RATIO) - 0.0827f) / -0.4807f);
    wspOk = isfinite(ppm) && ppm >= 0.0f && ppm <= 1000.0f;
    return wspOk ? ppm : NAN;
}

void initializeSensors() {
    scdOk = scd30.begin();
    if (scdOk) scd30.setMeasurementInterval(2);

    // Le HM3301 peut ignorer la première commande juste après l'alimentation.
    // Les lectures suivantes retenteront aussi automatiquement l'initialisation.
    delay(250);
    hmOk = initializeHm3301();
    sgpOk = sgp40.begin();
    dht20.begin();
    dhtOk = true;

#if EASE_WSP2110_ENABLED
    pinMode(EASE_WSP2110_PIN, INPUT);
    analogReadResolution(12);
    analogSetPinAttenuation(EASE_WSP2110_PIN, ADC_11db);
    currentMeasurement.hcho = readWsp2110();
#else
    currentMeasurement.hcho = NAN;
    wspRsRatio = NAN;
    wspAdcVoltage = NAN;
    wspOk = false;
#endif

    Serial.printf("Capteurs SCD30=%d HM3301=%d SGP40=%d DHT20=%d\n",
                  scdOk, hmOk, sgpOk, dhtOk);
#if EASE_WSP2110_ENABLED
    Serial.printf("WSP2110=%d\n", wspOk);
#else
    Serial.println("WSP2110=desactive (D0 inutilise)");
#endif
}

void readSensors() {
    if (dht20.read() == DHT20_OK) {
        currentMeasurement.temperature = dht20.getTemperature();
        currentMeasurement.humidity = dht20.getHumidity();
        dhtOk = isfinite(currentMeasurement.temperature) && isfinite(currentMeasurement.humidity);
    } else {
        dhtOk = false;
    }

    if (scd30.dataAvailable()) {
        float value = scd30.getCO2();
        if (value >= 0 && value <= 40000) {
            currentMeasurement.co2 = value;
            scdOk = true;
        }
    }

    if (readHm3301(hmBuffer, sizeof(hmBuffer))) {
        // Concentrations pour l'environnement atmosphérique. Les octets 4 à 9
        // contiennent les valeurs CF=1 destinées aux poussières industrielles.
        currentMeasurement.pm1 = static_cast<uint16_t>(hmBuffer[10] << 8 | hmBuffer[11]);
        currentMeasurement.pm25 = static_cast<uint16_t>(hmBuffer[12] << 8 | hmBuffer[13]);
        currentMeasurement.pm10 = static_cast<uint16_t>(hmBuffer[14] << 8 | hmBuffer[15]);
        if (hmConsecutiveFailures > 0) {
            Serial.printf("HM3301 communication retablie apres %u echec(s)\n",
                          hmConsecutiveFailures);
        }
        hmConsecutiveFailures = 0;
        hmHasValidFrame = true;
        hmOk = true;
    } else {
        if (hmConsecutiveFailures < 255) hmConsecutiveFailures++;
        // Tolérer deux erreurs transitoires après une mesure valide et
        // conserver les dernières valeurs affichées.
        if (!hmHasValidFrame || hmConsecutiveFailures >= 3) hmOk = false;
        if (hmConsecutiveFailures == 1 || hmConsecutiveFailures % 6 == 0) {
            Serial.printf("HM3301 lecture invalide: %u octet(s), echec consecutif %u; nouvelle initialisation\n",
                          static_cast<unsigned>(hmLastReceivedBytes),
                          hmConsecutiveFailures);
        }
        initializeHm3301();
    }

#if EASE_WSP2110_ENABLED
    currentMeasurement.hcho = readWsp2110();
#else
    currentMeasurement.hcho = NAN;
#endif
}

void readSgp40() {
    if (!sgpOk) return;
    float temperature = isfinite(currentMeasurement.temperature) ? currentMeasurement.temperature : 25.0f;
    float humidity = isfinite(currentMeasurement.humidity) ? currentMeasurement.humidity : 50.0f;
    int32_t index = sgp40.measureVocIndex(temperature, humidity);
    if (index >= 0 && index <= 500) currentMeasurement.voc = static_cast<float>(index);
}

uint8_t sensorOkCount() {
    uint8_t count = static_cast<uint8_t>(scdOk) + static_cast<uint8_t>(hmOk) +
                    static_cast<uint8_t>(sgpOk) + static_cast<uint8_t>(dhtOk);
#if EASE_WSP2110_ENABLED
    count += static_cast<uint8_t>(wspOk);
#endif
    return count;
}

void takeSample() {
    readSensors();
    currentMeasurement.timestamp = nowEpoch();
    currentMeasurement.quality = computeQuality();
    currentMeasurement.ventilation = ventilationActive;
    appendHistory(currentMeasurement);
    updateAllEvents(currentMeasurement.timestamp);
    updateAlarm();
    logMeasurement(currentMeasurement);
    Serial.printf("%s CO2=%.0f VOC=%.0f HCHO_est=%.1fppm WSP_Rs=%.2f ADC=%.3fV PM2.5=%.1f HM=%d T=%.1f RH=%.1f Q=%u\n",
                  localIso(currentMeasurement.timestamp).c_str(), currentMeasurement.co2,
                  currentMeasurement.voc, currentMeasurement.hcho, wspRsRatio, wspAdcVoltage,
                  currentMeasurement.pm25, hmOk,
                  currentMeasurement.temperature, currentMeasurement.humidity,
                  currentMeasurement.quality);
}

// -----------------------------------------------------------------------------
// OLED
// -----------------------------------------------------------------------------

void printOledValue(uint8_t x, uint8_t y, const char* label, float value, uint8_t decimals = 0) {
    char buffer[17];
    if (isfinite(value)) snprintf(buffer, sizeof(buffer), "%s %.*f", label, decimals, value);
    else snprintf(buffer, sizeof(buffer), "%s --", label);
    oled.setCursor(x, y);
    oled.print(buffer);
}

void drawOled() {
    oled.clear();
    char line[17];
    snprintf(line, sizeof(line), "EASE %s", alarmActive ? "ALERTE" : "AIR");
    oled.setCursor(0, 0);
    oled.print(line);
    oled.setCursor(11, 0);
    if (currentMeasurement.timestamp > 946684800UL) {
        time_t raw = currentMeasurement.timestamp;
        struct tm localValue;
        localtime_r(&raw, &localValue);
        snprintf(line, sizeof(line), "%02d:%02d", localValue.tm_hour, localValue.tm_min);
        oled.print(line);
    }

    switch (oledPage) {
        case 0:
            oled.setCursor(0, 2); oled.print("GAZ / COMPOSES");
            printOledValue(0, 3, "CO2", currentMeasurement.co2);
            printOledValue(0, 4, "VOC", currentMeasurement.voc);
            printOledValue(0, 5, "HCHOe", currentMeasurement.hcho, 1);
            break;
        case 1:
            oled.setCursor(0, 2); oled.print("PARTICULES");
            printOledValue(0, 3, "PM1", currentMeasurement.pm1);
            printOledValue(0, 4, "PM2.5", currentMeasurement.pm25);
            printOledValue(0, 5, "PM10", currentMeasurement.pm10);
            printOledValue(0, 6, "Moy24h", pm25Average24h(), 1);
            break;
        case 2:
            oled.setCursor(0, 2); oled.print("CONFORT");
            printOledValue(0, 3, "Temp C", currentMeasurement.temperature, 1);
            printOledValue(0, 4, "Hum %", currentMeasurement.humidity, 1);
            oled.setCursor(0, 6); oled.print(qualityCause);
            break;
        default:
            oled.setCursor(0, 2); oled.print(apMode ? "MODE POINT ACCES" : "MODE WIFI");
            oled.setCursor(0, 3); oled.print(WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : WiFi.softAPIP().toString());
            oled.setCursor(0, 5); oled.print(sdOk ? "SD OK" : "SD ERREUR");
            oled.setCursor(8, 5); oled.print(rtcOk ? "RTC OK" : "RTC ERR");
            snprintf(line, sizeof(line), "CAPTEURS %u/%u", sensorOkCount(), SENSOR_TOTAL);
            oled.setCursor(0, 6); oled.print(line);
            break;
    }
    oled.setCursor(0, 7);
    oled.print("Bon Acc Deg ALR");
    oled.setCursor(currentMeasurement.quality * 4 > 12 ? 12 : currentMeasurement.quality * 4, 7);
    oled.print("^");
}

void serviceOledButton(uint32_t now) {
    bool reading = digitalRead(EASE_OLED_BUTTON_PIN) == HIGH;
    if (reading != buttonLastReading) {
        buttonLastReading = reading;
        lastButtonChange = now;
    }

    if (now - lastButtonChange >= EASE_BUTTON_DEBOUNCE_MS &&
        reading != buttonStableState) {
        buttonStableState = reading;
        if (buttonStableState) {
            oledPage = (oledPage + 1) % 4;
            lastOledActivity = now;
            if (!oledAwake) {
                oled.setPowerSave(0);
                oledAwake = true;
            }
            drawOled();
        }
    }

    if (oledAwake && now - lastOledActivity >= EASE_OLED_TIMEOUT_MS) {
        oled.setPowerSave(1);
        oledAwake = false;
    }
}

void serviceVentilationButton(uint32_t now) {
    bool reading = digitalRead(EASE_VENTILATION_BUTTON_PIN) == HIGH;
    if (reading != ventilationButtonLastReading) {
        ventilationButtonLastReading = reading;
        lastVentilationButtonChange = now;
    }

    if (now - lastVentilationButtonChange >= EASE_BUTTON_DEBOUNCE_MS &&
        reading != ventilationButtonStableState) {
        ventilationButtonStableState = reading;
        if (ventilationButtonStableState) {
            ventilationActive = !ventilationActive;
            Serial.printf("Ventilation: %s\n", ventilationActive ? "ACTIVE" : "ARRETEE");
        }
    }
}

// -----------------------------------------------------------------------------
// Réseau et API HTTP
// -----------------------------------------------------------------------------

void startAccessPoint() {
    WiFi.mode(WIFI_AP);
    apMode = true;
    WiFi.softAP("EASE-XIAO", EASE_AP_PASSWORD);
    Serial.printf("Point d'acces EASE-XIAO: http://%s\n", WiFi.softAPIP().toString().c_str());
}

void initializeNetwork() {
    WiFi.setHostname(EASE_HOSTNAME);
    if (strlen(EASE_WIFI_SSID) == 0) {
        startAccessPoint();
        return;
    }
    WiFi.mode(WIFI_STA);
    WiFi.begin(EASE_WIFI_SSID, EASE_WIFI_PASSWORD);
    for (uint8_t i = 0; i < 24 && WiFi.status() != WL_CONNECTED; i++) delay(500);
    if (WiFi.status() == WL_CONNECTED) {
        apMode = false;
        Serial.printf("Wi-Fi connecte: http://%s\n", WiFi.localIP().toString().c_str());
    } else {
        WiFi.disconnect(true);
        startAccessPoint();
    }
    if (MDNS.begin(EASE_HOSTNAME)) MDNS.addService("http", "tcp", 80);
}

const char* qualityLabel(uint8_t quality) {
    static const char* labels[] = {"Bon", "Acceptable", "Degrade", "Alerte"};
    return labels[quality > 3 ? 3 : quality];
}

void handleLive() {
    String json;
    json.reserve(1000);
    String ip = apMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
    json += "{\"timestamp\":" + String(currentMeasurement.timestamp);
    json += ",\"iso\":\"" + localIso(currentMeasurement.timestamp) + "\"";
    json += ",\"co2\":" + jsonNumber(currentMeasurement.co2, 0);
    json += ",\"voc\":" + jsonNumber(currentMeasurement.voc, 0);
    json += ",\"hcho\":" + jsonNumber(currentMeasurement.hcho, 1);
    json += ",\"hcho_enabled\":" + String(EASE_WSP2110_ENABLED ? "true" : "false");
    json += ",\"wsp_rs_ratio\":" + jsonNumber(wspRsRatio, 2);
    json += ",\"wsp_adc_v\":" + jsonNumber(wspAdcVoltage, 3);
    json += ",\"pm1\":" + jsonNumber(currentMeasurement.pm1, 0);
    json += ",\"pm25\":" + jsonNumber(currentMeasurement.pm25, 0);
    json += ",\"pm10\":" + jsonNumber(currentMeasurement.pm10, 0);
    json += ",\"temp\":" + jsonNumber(currentMeasurement.temperature, 1);
    json += ",\"humidity\":" + jsonNumber(currentMeasurement.humidity, 1);
    json += ",\"pm25_24h\":" + jsonNumber(pm25Average24h(), 1);
    json += ",\"pm25_24h_limit\":" + String(EASE_PM25_24H_LIMIT_UGM3, 1);
    json += ",\"pm25_24h_ready\":" + String(pm25AverageReady() ? "true" : "false");
    json += ",\"history_seconds\":" + String(historyCoverageSeconds());
    json += ",\"history_count\":" + String(historyCount);
    json += ",\"quality\":" + String(currentMeasurement.quality);
    json += ",\"quality_label\":\"" + String(qualityLabel(currentMeasurement.quality)) + "\"";
    json += ",\"quality_cause\":\"" + String(qualityCause) + "\"";
    json += ",\"alarm\":" + String(alarmActive ? "true" : "false");
    json += ",\"ventilation_active\":" + String(ventilationActive ? "true" : "false");
    json += ",\"i2c_hub_ok\":" + String(i2cMuxOk ? "true" : "false");
    json += ",\"rtc_ok\":" + String(rtcOk ? "true" : "false");
    json += ",\"sd_ok\":" + String(sdOk ? "true" : "false");
    json += ",\"ap_mode\":" + String(apMode ? "true" : "false");
    json += ",\"wifi_rssi\":" + String(apMode ? 0 : WiFi.RSSI());
    json += ",\"ip\":\"" + ip + "\"";
    json += ",\"sensors_ok\":" + String(sensorOkCount());
    json += ",\"sensors_total\":" + String(SENSOR_TOTAL) + "}";
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "application/json; charset=utf-8", json);
}

void handleHistory() {
    uint32_t since = server.hasArg("since") ? strtoul(server.arg("since").c_str(), nullptr, 10) : 0;
    long requestedMaximum = server.hasArg("max") ? server.arg("max").toInt() : 900;
    if (requestedMaximum < 50) requestedMaximum = 50;
    if (requestedMaximum > 1200) requestedMaximum = 1200;
    size_t maximum = static_cast<size_t>(requestedMaximum);
    size_t eligible = 0;
    for (size_t i = 0; i < historyCount; i++) if (historyAt(i).timestamp >= since) eligible++;
    size_t stride = eligible > maximum ? (eligible + maximum - 1) / maximum : 1;

    server.sendHeader("Cache-Control", "no-store");
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "application/json; charset=utf-8", "");
    server.sendContent("{\"points\":[");
    bool first = true;
    size_t selected = 0;
    for (size_t i = 0; i < historyCount; i++) {
        Measurement& m = historyAt(i);
        if (m.timestamp < since) continue;
        bool ventilationChanged = i > 0 && historyAt(i - 1).ventilation != m.ventilation;
        if ((selected++ % stride) != 0 && !ventilationChanged && i + 1 < historyCount) continue;
        String item;
        item.reserve(190);
        if (!first) item += ',';
        first = false;
        item += "{\"t\":" + String(m.timestamp);
        item += ",\"co2\":" + jsonNumber(m.co2, 0);
        item += ",\"voc\":" + jsonNumber(m.voc, 0);
        item += ",\"hcho\":" + jsonNumber(m.hcho, 1);
        item += ",\"pm1\":" + jsonNumber(m.pm1, 0);
        item += ",\"pm25\":" + jsonNumber(m.pm25, 0);
        item += ",\"pm10\":" + jsonNumber(m.pm10, 0);
        item += ",\"temp\":" + jsonNumber(m.temperature, 1);
        item += ",\"humidity\":" + jsonNumber(m.humidity, 1);
        item += ",\"ventilation\":" + String(m.ventilation ? "true" : "false") + "}";
        server.sendContent(item);
        delay(0);
    }
    server.sendContent("]}");
    server.sendContent("");
}

void handleEvents() {
    long requestedMaximum = server.hasArg("max") ? server.arg("max").toInt() : 100;
    if (requestedMaximum < 10) requestedMaximum = 10;
    if (requestedMaximum > 200) requestedMaximum = 200;
    size_t maximum = static_cast<size_t>(requestedMaximum);
    size_t start = eventCount > maximum ? eventCount - maximum : 0;
    server.sendHeader("Cache-Control", "no-store");
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "application/json; charset=utf-8", "");
    server.sendContent("{\"events\":[");
    bool first = true;
    for (size_t i = start; i < eventCount; i++) {
        EventRecord& event = eventAt(i);
        String item;
        item.reserve(220);
        if (!first) item += ',';
        first = false;
        item += "{\"start\":" + String(event.start);
        item += ",\"end\":" + String(event.end);
        item += ",\"duration\":" + String(event.end - event.start);
        item += ",\"parameter\":\"" + String(event.parameter) + "\"";
        item += ",\"unit\":\"" + String(event.unit) + "\"";
        item += ",\"threshold\":" + String(event.threshold, 2);
        item += ",\"average\":" + String(event.average, 2);
        item += ",\"maximum\":" + String(event.maximum, 2);
        item += ",\"peak\":" + String(event.peakAt) + "}";
        server.sendContent(item);
    }
    server.sendContent("]}");
    server.sendContent("");
}

void handleEventsCsv() {
    if (!sdOk || !SD.exists("/events.csv")) {
        server.send(404, "text/plain; charset=utf-8", "Aucun evenement enregistre");
        return;
    }
    File file = SD.open("/events.csv", FILE_READ);
    server.sendHeader("Content-Disposition", "attachment; filename=ease_evenements.csv");
    server.streamFile(file, "text/csv; charset=utf-8");
    file.close();
}

void initializeServer() {
    server.on("/", HTTP_GET, []() {
        server.sendHeader("Cache-Control", "no-cache");
        server.send_P(200, "text/html; charset=utf-8", EASE_DASHBOARD_HTML);
    });
    server.on("/api/live", HTTP_GET, handleLive);
    server.on("/api/history", HTTP_GET, handleHistory);
    server.on("/api/events", HTTP_GET, handleEvents);
    server.on("/api/events.csv", HTTP_GET, handleEventsCsv);
    server.on("/api/health", HTTP_GET, []() {
        String response = String("{\"ok\":true,\"uptime_s\":") + millis() / 1000UL +
                          ",\"free_heap\":" + ESP.getFreeHeap() +
                          ",\"free_psram\":" + ESP.getFreePsram() + "}";
        server.send(200, "application/json", response);
    });
    server.onNotFound([]() { server.send(404, "text/plain", "EASE: page introuvable"); });
    server.begin();
}

void serviceNetwork() {
    server.handleClient();
    if (!apMode && WiFi.status() != WL_CONNECTED && millis() - lastWifiAttempt > 30000UL) {
        lastWifiAttempt = millis();
        WiFi.reconnect();
    }
    if (!apMode && WiFi.status() == WL_CONNECTED && millis() - lastRtcNtpWrite > 6UL * 3600UL * 1000UL) {
        time_t epoch = time(nullptr);
        if (epoch > 1700000000) {
            rtcOk = writeRtc(epoch);
            lastRtcNtpWrite = millis();
        }
    }
}

// -----------------------------------------------------------------------------
// Arduino
// -----------------------------------------------------------------------------

void setup() {
    Serial.begin(115200);
    delay(250);
    pinMode(EASE_BUZZER_PIN, OUTPUT);
    noTone(EASE_BUZZER_PIN);
    pinMode(EASE_OLED_BUTTON_PIN, INPUT);
    buttonStableState = digitalRead(EASE_OLED_BUTTON_PIN) == HIGH;
    buttonLastReading = buttonStableState;
    lastButtonChange = millis();
    pinMode(EASE_VENTILATION_BUTTON_PIN, INPUT);
    ventilationButtonStableState = digitalRead(EASE_VENTILATION_BUTTON_PIN) == HIGH;
    ventilationButtonLastReading = ventilationButtonStableState;
    lastVentilationButtonChange = millis();
    lastOledActivity = millis();

    Wire.begin(D4, D5);
    Wire.setClock(100000);  // fréquence sûre pour l'ensemble des modules Grove
    i2cMuxOk = initializeI2cMultiplexer();
    delay(10);
    oled.begin();
    oled.setPowerSave(0);
    oled.setFont(u8x8_font_chroma48medium8_r);
    oled.clear();
    oled.setCursor(0, 0); oled.print("EASE XIAO S3");
    oled.setCursor(0, 2); oled.print("Initialisation");

    historyBuffer = static_cast<Measurement*>(ps_malloc(sizeof(Measurement) * HISTORY_CAPACITY));
    eventBuffer = static_cast<EventRecord*>(ps_malloc(sizeof(EventRecord) * EVENT_CAPACITY));
    if (!historyBuffer || !eventBuffer) {
        Serial.println("ERREUR: PSRAM non disponible. Activez OPI PSRAM dans Arduino IDE.");
        oled.setCursor(0, 4); oled.print("PSRAM ERREUR");
        while (true) delay(1000);
    }

    initializeClockFromRtc();
    initializeSensors();
    initializeStorage();
    initializeNetwork();
    synchronizeNtp();
    initializeServer();

    takeSample();
    drawOled();
    lastOledActivity = millis();
}

void loop() {
    uint32_t now = millis();
    serviceNetwork();
    serviceBuzzer();

    if (now - lastSgp40 >= EASE_SGP40_INTERVAL_MS) {
        lastSgp40 = now;
        readSgp40();
    }
    if (now - lastSample >= EASE_SAMPLE_INTERVAL_MS) {
        lastSample = now;
        takeSample();
        if (oledAwake) drawOled();
    }
    serviceOledButton(now);
    serviceVentilationButton(now);
    delay(2);
}
