#ifndef EASE_HARDWARE_H
#define EASE_HARDWARE_H

// EASE R10.35 — pilotes I2C consolides, audit des calculs et optimisation Uno.
// Wire reste la bibliothèque Arduino officielle; ce fichier regroupe uniquement
// le hub TCA9548A et les drivers validés DPS310 / SGP30 / Gas v2.

#include <Arduino.h>
#include <Wire.h>

// Grove 8 Channel I2C Hub (TCA9548A), adresse 0x70.
// Mapping confirme par le diagnostic du 21/06/2026 :
//   Gas v2 0x08 -> canal 5
//   SGP30  0x58 -> canal 6
//   DPS310 0x77 -> canal 7
#define USE_I2C_MUX 1

const uint8_t MUX_ADDR = 0x70;
const uint8_t MUX_CH_NONE = 0xFF;
// IMPORTANT : Si7021 et HM3301 utilisent tous deux l'adresse I2C 0x40.
// Avec un seul bus Uno, le Si7021 doit donc etre isole par le TCA9548A :
// Si7021 canal 3, HM3301 canal 4. Ne pas mettre le Si7021 directement sur le
// bus principal tant que le HM3301 reste sur ce meme TCA9548A.
#define SI7021_ON_MUX 1
const uint8_t MUX_CH_SI7021 = 3;
const uint8_t MUX_CH_HM3301 = 4; // Grove Laser PM2.5 Sensor HM3301
const uint8_t MUX_CH_GASV2 = 5;
const uint8_t MUX_CH_SGP30 = 6;
const uint8_t MUX_CH_BARO  = 7;

static uint8_t i2cMuxActiveCh = MUX_CH_NONE;

static inline bool muxPresent() {
#if USE_I2C_MUX
  Wire.beginTransmission(MUX_ADDR);
  return Wire.endTransmission() == 0;
#else
  return false;
#endif
}

static inline bool muxSelectRaw(uint8_t ch) {
#if USE_I2C_MUX
  Wire.beginTransmission(MUX_ADDR);
  Wire.write(ch > 7 ? 0 : (uint8_t)(1U << ch));
  if (Wire.endTransmission() != 0) return false;
  i2cMuxActiveCh = ch;
  return true;
#else
  (void)ch;
  return true;
#endif
}

// Force toujours l'etat physique du TCA9548A a "aucun canal". Ne pas
// s'appuyer uniquement sur i2cMuxActiveCh : une transaction interrompue peut
// desynchroniser l'etat logiciel du hub reel.
static void __attribute__((noinline)) muxDeselect() {
#if USE_I2C_MUX
  Wire.beginTransmission(MUX_ADDR);
  Wire.write((uint8_t)0x00);
  Wire.endTransmission();
  i2cMuxActiveCh = MUX_CH_NONE;
#endif
}

static bool __attribute__((noinline)) muxSelectFor(uint8_t ch) {
#if USE_I2C_MUX
  if (ch == MUX_CH_NONE) {
    muxDeselect();
    return true;
  }
  if (i2cMuxActiveCh == ch) return true;
  return muxSelectRaw(ch);
#else
  (void)ch;
  return true;
#endif
}

const uint8_t DPS310_ADDR = 0x77;
const uint8_t SGP30_ADDR = 0x58;
const uint8_t GASV2_ADDR = 0x08;
const uint8_t HM3301_ADDR = 0x40;

const unsigned long SGP30_WARMUP_MS = 15000UL;
const unsigned long GASV2_WARMUP_MS = 300000UL;
const unsigned long SGP30_STALE_MS = 5000UL;
const unsigned long GASV2_POLL_MS = 5000UL;
const unsigned long GASV2_STALE_MS = 15000UL;
const unsigned long HM3301_WARMUP_MS = 30000UL;
const unsigned long HM3301_POLL_MS = 5000UL;
const unsigned long HM3301_STALE_MS = 15000UL;

static bool __attribute__((noinline)) i2cWrite8(uint8_t addr, uint8_t reg, uint8_t val) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.write(val);
  return Wire.endTransmission() == 0;
}

static bool __attribute__((noinline)) i2cReadReg(uint8_t addr, uint8_t reg, uint8_t* buf, uint8_t len) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((int)addr, (int)len) != len) {
    while (Wire.available()) Wire.read();
    return false;
  }
  for (uint8_t i = 0; i < len; i++) {
    if (!Wire.available()) return false;
    buf[i] = Wire.read();
  }
  return true;
}

static bool __attribute__((noinline)) i2cWriteCmd(uint8_t addr, const uint8_t* cmd, uint8_t len) {
  Wire.beginTransmission(addr);
  for (uint8_t i = 0; i < len; i++) Wire.write(cmd[i]);
  return Wire.endTransmission() == 0;
}

static bool __attribute__((noinline)) i2cReadBytes(uint8_t addr, uint8_t* buf, uint8_t len) {
  if (Wire.requestFrom((int)addr, (int)len) != len) {
    while (Wire.available()) Wire.read();
    return false;
  }
  for (uint8_t i = 0; i < len; i++) {
    if (!Wire.available()) return false;
    buf[i] = Wire.read();
  }
  return true;
}

static bool __attribute__((noinline)) i2cPresent(uint8_t addr) {
  Wire.beginTransmission(addr);
  return Wire.endTransmission() == 0;
}

// CRC Sensirion / Si7021, polynome 0x31. L'initialisation depend du capteur :
// 0x00 pour le Si7021 et 0xFF pour le SGP30.
static uint8_t __attribute__((noinline)) easeCrc31(const uint8_t* data, uint8_t length, uint8_t crc) {
  while (length--) {
    crc ^= *data++;
    for (uint8_t bit = 0; bit < 8; bit++) {
      crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x31) : (uint8_t)(crc << 1);
    }
  }
  return crc;
}

// ---------------------------------------------------------------------------
// DPS310 : mesure continue 1 Hz, oversampling x1.
//
// Version Uno RAM/Flash : compensation complete en virgule fixe Q16.
// Le code precedent faisait les memes calculs en float; sur ATmega328P cela
// embarque la lourde bibliotheque logicielle de calcul flottant. Ici pQ/tQ
// representent Praw/k et Traw/k avec 16 bits de fraction (k = 524288).
// Aucun flottant n'est utilise : pression conservee en hPa x10.
// ---------------------------------------------------------------------------
struct Dps310State {
  int32_t c00, c10;
  int16_t c01, c11, c20, c21, c30;
  bool ready;
};
static Dps310State dps310 = {0, 0, 0, 0, 0, 0, 0, false};

static int32_t dps310SignExtend(int32_t value, uint8_t bits) {
  const int32_t signBit = (int32_t)1 << (bits - 1);
  return (value ^ signBit) - signBit;
}

static int32_t dps310Read24Signed(const uint8_t* b) {
  int32_t v = ((int32_t)b[0] << 16) | ((int32_t)b[1] << 8) | b[2];
  if (v & 0x800000L) v |= 0xFF000000L;
  return v;
}

static bool dps310WaitStatus(uint8_t requiredBits, unsigned long timeoutMs) {
  const unsigned long start = millis();
  uint8_t status = 0;
  while (millis() - start < timeoutMs) {
    if (i2cReadReg(DPS310_ADDR, 0x08, &status, 1) &&
        (status & requiredBits) == requiredBits) return true;
    delay(5);
  }
  return false;
}

static bool dps310LoadCoef() {
  uint8_t b[18];
  if (!i2cReadReg(DPS310_ADDR, 0x10, b, sizeof(b))) return false;
  // c0/c1 (temperature) ne sont pas necessaires : la temperature de reference
  // affichee est celle du Si7021. Les coefficients de pression restent tous lus.
  dps310.c00 = dps310SignExtend(((int32_t)b[3] << 12) | ((int32_t)b[4] << 4) | (b[5] >> 4), 20);
  dps310.c10 = dps310SignExtend((((int32_t)b[5] & 0x0F) << 16) | ((int32_t)b[6] << 8) | b[7], 20);
  dps310.c01 = (int16_t)(((uint16_t)b[8] << 8) | b[9]);
  dps310.c11 = (int16_t)(((uint16_t)b[10] << 8) | b[11]);
  dps310.c20 = (int16_t)(((uint16_t)b[12] << 8) | b[13]);
  dps310.c21 = (int16_t)(((uint16_t)b[14] << 8) | b[15]);
  dps310.c30 = (int16_t)(((uint16_t)b[16] << 8) | b[17]);
  return true;
}

static bool dps310Configure() {
  uint8_t coefSource = 0;
  if (!i2cReadReg(DPS310_ADDR, 0x28, &coefSource, 1)) return false;
  if (!i2cWrite8(DPS310_ADDR, 0x08, 0x00)) return false; // idle
  if (!i2cWrite8(DPS310_ADDR, 0x06, 0x00)) return false; // P 1 Hz, x1
  if (!i2cWrite8(DPS310_ADDR, 0x07, (coefSource & 0x80) ? 0x80 : 0x00)) return false; // T 1 Hz, x1
  if (!i2cWrite8(DPS310_ADDR, 0x09, 0x00)) return false; // no FIFO/no shift
  return i2cWrite8(DPS310_ADDR, 0x08, 0x07); // continuous P+T
}

// Produit signe Q16.16 sans int64 : (a*b)>>16, exact tant que le
// resultat Q16 tient dans int32_t. C'est le cas pour les coefficients et
// pressions physiques du DPS310 (la plage finale est ensuite verifiee).
// L'AVR evite ainsi les routines logicielles 64 bits tres couteuses en Flash.
static int32_t dpsMulQ16(int32_t a, int32_t b) {
  const int16_t ah = (int16_t)(a >> 16);
  const uint16_t al = (uint16_t)a;
  const int16_t bh = (int16_t)(b >> 16);
  const uint16_t bl = (uint16_t)b;
  return (int32_t)ah * b + (int32_t)bh * al + (int32_t)(((uint32_t)al * bl) >> 16);
}

static bool dps310Compensate(int32_t rawT, int32_t rawP, int& presHpa_x10) {
  const int32_t tQ = rawT / 8L;
  const int32_t pQ = rawP / 8L;
  int32_t poly = (int32_t)dps310.c20 + dpsMulQ16(dps310.c30, pQ);
  poly = dps310.c10 + dpsMulQ16(poly, pQ);
  int32_t pPa = dps310.c00 + dpsMulQ16(poly, pQ);
  pPa += dpsMulQ16(dps310.c01, tQ);
  int32_t cross = (int32_t)dps310.c11 + dpsMulQ16(dps310.c21, pQ);
  cross = dpsMulQ16(cross, pQ);
  pPa += dpsMulQ16(cross, tQ);
  if (pPa < 30000L || pPa > 120000L) return false;
  presHpa_x10 = (int)((pPa + 5L) / 10L);
  return true;
}

bool initDps310() {
  dps310.ready = false;
  if (!muxSelectFor(MUX_CH_BARO) || !i2cPresent(DPS310_ADDR)) return false;
  if (!i2cWrite8(DPS310_ADDR, 0x0C, 0x09)) return false; // soft reset
  if (!dps310WaitStatus(0xC0, 500)) return false; // SENSOR_RDY + COEF_RDY
  uint8_t id = 0;
  if (!i2cReadReg(DPS310_ADDR, 0x0D, &id, 1) || (id != 0x10 && id != 0x11)) return false;
  if (!dps310LoadCoef() || !dps310Configure()) return false;
  if (!dps310WaitStatus(0x30, 1500)) return false; // PRS_RDY + TMP_RDY
  dps310.ready = true;
  return true;
}

bool readDps310(int& presHpa_x10) {
  presHpa_x10 = -1;
  if (!dps310.ready || !muxSelectFor(MUX_CH_BARO)) return false;
  if (!dps310WaitStatus(0x30, 1500)) return false;
  uint8_t raw[6];
  if (!i2cReadReg(DPS310_ADDR, 0x00, raw, sizeof(raw))) return false;
  return dps310Compensate(dps310Read24Signed(raw + 3), dps310Read24Signed(raw), presHpa_x10);
}

// ---------------------------------------------------------------------------
// SGP30 : CRC Sensirion init 0xFF, acquisition continue 1 Hz + cache.
// Les valeurs fraiches restent accessibles pendant la phase CHAUFFE : le
// firmware d'interface affiche alors la valeur avec un statut non valide.
// ---------------------------------------------------------------------------
static bool sgp30Ready = false;
static unsigned long sgp30LastMeasureMs = 0;
static unsigned long sgp30LastValidMs = 0;
static int sgp30CachedTvoc = -1;

bool initSgp30() {
  sgp30Ready = false;
  sgp30CachedTvoc = -1;
  if (!muxSelectFor(MUX_CH_SGP30) || !i2cPresent(SGP30_ADDR)) return false;
  const uint8_t cmd[] = {0x20, 0x03}; // iaq_init
  if (!i2cWriteCmd(SGP30_ADDR, cmd, sizeof(cmd))) return false;
  delay(10);
  sgp30LastMeasureMs = millis() - 1000UL;
  sgp30LastValidMs = 0;
  sgp30Ready = true;
  return true;
}


void pollSgp30(unsigned long nowMs) {
  if (!sgp30Ready || nowMs - sgp30LastMeasureMs < 1000UL) return;
  sgp30LastMeasureMs = nowMs;
  if (!muxSelectFor(MUX_CH_SGP30)) return;
  const uint8_t cmd[] = {0x20, 0x08}; // measure_iaq
  if (!i2cWriteCmd(SGP30_ADDR, cmd, sizeof(cmd))) return;
  delay(12);
  uint8_t data[6];
  if (!i2cReadBytes(SGP30_ADDR, data, sizeof(data))) return;
  if (easeCrc31(data, 2, 0xFF) != data[2] || easeCrc31(data + 3, 2, 0xFF) != data[5]) return;
  const uint16_t tvoc = ((uint16_t)data[3] << 8) | data[4];
  // Le protocole publie uniquement TVOC. Le CRC valide aussi les octets eCO2 ;
  // ne pas decoder une valeur non utilisee economise pile et Flash.
  if (tvoc > 32767U) return;
  sgp30CachedTvoc = (int)tvoc;
  sgp30LastValidMs = nowMs;
}

bool readSgp30(int& tvocPpb, unsigned long nowMs) {
  tvocPpb = -1;
  pollSgp30(nowMs);
  if (!sgp30LastValidMs || nowMs - sgp30LastValidMs > SGP30_STALE_MS) return false;
  tvocPpb = sgp30CachedTvoc;
  return true;
}

// ---------------------------------------------------------------------------
// Grove Multichannel Gas Sensor v2 : valeurs brutes qualitatives, cache 5 s.
// ---------------------------------------------------------------------------
static bool gasV2Ready = false;
static unsigned long gasV2LastPollMs = 0;
static unsigned long gasV2LastValidMs = 0;
static int gasV2CachedNo2 = -1, gasV2CachedCo = -1;

const uint8_t GM_102B = 0x01;
const uint8_t GM_702B = 0x07;
const uint8_t GAS_WARMING_UP = 0xFE;

static bool gasV2WriteByte(uint8_t cmd) {
  Wire.beginTransmission(GASV2_ADDR);
  Wire.write(cmd);
  bool ok = Wire.endTransmission() == 0;
  delay(1);
  return ok;
}

static bool gasV2Read32(uint32_t& value) {
  value = 0;
  if (Wire.requestFrom((int)GASV2_ADDR, (int)4) != 4) {
    while (Wire.available()) Wire.read();
    return false;
  }
  for (uint8_t index = 0; index < 4; index++) {
    if (!Wire.available()) return false;
    value |= (uint32_t)Wire.read() << (8 * index);
  }
  delay(1);
  return true;
}

static bool gasV2ReadChannel(uint8_t cmd, uint32_t& value) {
  if (!gasV2WriteByte(cmd)) return false;
  return gasV2Read32(value);
}

static int gasV2Clamp999(uint32_t raw) {
  return raw > 999UL ? 999 : (int)raw;
}

bool initGasV2() {
  gasV2Ready = false;
  gasV2CachedNo2 = gasV2CachedCo = -1;
  gasV2LastPollMs = millis() - GASV2_POLL_MS;
  gasV2LastValidMs = 0;
  if (!muxSelectFor(MUX_CH_GASV2) || !i2cPresent(GASV2_ADDR)) return false;
  if (!gasV2WriteByte(GAS_WARMING_UP)) return false;
  gasV2Ready = true;
  return true;
}


void pollGasV2(unsigned long nowMs) {
  if (!gasV2Ready || nowMs - gasV2LastPollMs < GASV2_POLL_MS) return;
  gasV2LastPollMs = nowMs;
  if (!muxSelectFor(MUX_CH_GASV2)) return;
  uint32_t no2, co;
  if (!gasV2ReadChannel(GM_102B, no2) || !gasV2ReadChannel(GM_702B, co)) return;
  gasV2CachedNo2 = gasV2Clamp999(no2);
  gasV2CachedCo = gasV2Clamp999(co);
  gasV2LastValidMs = nowMs;
}

bool readGasV2(int& no2, int& co, unsigned long nowMs) {
  no2 = co = -1;
  pollGasV2(nowMs);
  if (!gasV2LastValidMs || nowMs - gasV2LastValidMs > GASV2_STALE_MS) return false;
  no2 = gasV2CachedNo2;
  co = gasV2CachedCo;
  return true;
}


// ---------------------------------------------------------------------------
// Grove Laser PM2.5 Sensor HM3301 : frame of 29 bytes, checksum at byte 28.
// At power-up the sensor defaults to UART. Command 0x88 switches it to I2C.
// The official Seeed frame parser ignores bytes 0..1, then reads words at
// [2..3] (sensor number), [4..9] (CF=1) and [10..15] (ambient PM1/PM2.5/PM10).
// Frame is consumed in streaming mode to avoid a permanent 29-byte RAM buffer.
// ---------------------------------------------------------------------------
static bool hm3301Ready = false;
static unsigned long hm3301InitMs = 0;
static unsigned long hm3301LastPollMs = 0;
static unsigned long hm3301LastValidMs = 0;
static int hm3301CachedPm1 = -1, hm3301CachedPm25 = -1, hm3301CachedPm10 = -1;
// Nombre de trames non valides consecutives. Le module est remis en mode I2C
// apres deux echecs : cela traite une perte de mode apres un bruit / reset local
// sans accepter de donnees dont le checksum est faux.
static uint8_t hm3301BadFrames = 0;

static bool hm3301EnterI2cMode() {
  // Datasheet Appendix 2: after power-on, send byte 0x88 to the 7-bit address
  // 0x40. Without this command the sensor still acknowledges but can return
  // invalid frames/checksums because UART remains active.
  Wire.beginTransmission(HM3301_ADDR);
  Wire.write((uint8_t)0x88);
  if (Wire.endTransmission() != 0) return false;
  delay(20);
  return true;
}

bool initHm3301(unsigned long startMs) {
  hm3301Ready = false;
  hm3301CachedPm1 = hm3301CachedPm25 = hm3301CachedPm10 = -1;
  hm3301InitMs = startMs;
  hm3301LastPollMs = startMs - HM3301_POLL_MS;
  hm3301LastValidMs = 0;
  hm3301BadFrames = 0;
  if (!muxSelectFor(MUX_CH_HM3301) || !i2cPresent(HM3301_ADDR)) return false;
  delay(2); // laisse le TCA9548A stabiliser la route avant 0x88
  if (!hm3301EnterI2cMode()) return false;
  hm3301Ready = true;
  return true;
}

bool hm3301WarmedUp(unsigned long nowMs) {
  return hm3301Ready && (nowMs - hm3301InitMs >= HM3301_WARMUP_MS);
}

void pollHm3301(unsigned long nowMs) {
  if (!hm3301Ready || nowMs - hm3301LastPollMs < HM3301_POLL_MS) return;
  hm3301LastPollMs = nowMs;
  if (!muxSelectFor(MUX_CH_HM3301)) return;
  // Le buffer Wire de l'Uno contient 32 octets : 29 sont donc possibles.
  // On attend brievement la disponibilite complete au lieu de valider une
  // trame partielle, puis on vide toujours le buffer en cas d'erreur.
  delay(2);
  uint8_t received = (uint8_t)Wire.requestFrom((int)HM3301_ADDR, 29);
  for (uint8_t wait = 0; Wire.available() < 29 && wait < 8; wait++) delay(1);
  if (received != 29 || Wire.available() != 29) {
    while (Wire.available()) Wire.read();
    if (++hm3301BadFrames >= 2) { hm3301EnterI2cMode(); hm3301BadFrames = 0; }
    return;
  }
  uint8_t checksum = 0, expected = 0;
  bool nonZero = false;
  int pm1 = 0, pm25 = 0, pm10 = 0;
  for (uint8_t i = 0; i < 29; i++) {
    const uint8_t v = (uint8_t)Wire.read();
    if (i < 28) { checksum = (uint8_t)(checksum + v); if (v) nonZero = true; }
    else expected = v;
    // PM atmospheriques : conventions du parseur officiel Seeed.
    if (i == 10) pm1 = (int)v << 8;
    else if (i == 11) pm1 |= v;
    else if (i == 12) pm25 = (int)v << 8;
    else if (i == 13) pm25 |= v;
    else if (i == 14) pm10 = (int)v << 8;
    else if (i == 15) pm10 |= v;
  }
  // Important : les octets 0 et 1 ne sont pas un en-tete a valider. Sur le
  // HM3301 observe ici, ils valent 00 FF et le checksum est correct. Le
  // parseur officiel Seeed les ignore lui aussi. Une trame réellement nulle,
  // une longueur incomplete ou un checksum faux reste refusee.
  if (!nonZero || checksum != expected || pm1 > 2000 || pm25 > 2000 || pm10 > 2000) {
    if (++hm3301BadFrames >= 2) { hm3301EnterI2cMode(); hm3301BadFrames = 0; }
    return;
  }
  hm3301BadFrames = 0;
  hm3301CachedPm1 = pm1;
  hm3301CachedPm25 = pm25;
  hm3301CachedPm10 = pm10;
  hm3301LastValidMs = nowMs;
}

bool readHm3301(int& pm1, int& pm25, int& pm10, unsigned long nowMs) {
  pm1 = pm25 = pm10 = -1;
  pollHm3301(nowMs);
  if (!hm3301LastValidMs || nowMs - hm3301LastValidMs > HM3301_STALE_MS) return false;
  pm1 = hm3301CachedPm1;
  pm25 = hm3301CachedPm25;
  pm10 = hm3301CachedPm10;
  return true;
}

#endif
