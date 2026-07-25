/*
  Projet EASE R10.44 — RTC : horodatage CSV rétabli — Arduino Uno

  Base de stabilité : R10.12 RAM-FIRST (PEND.DAT binaire validé).
  Fiabilisations R10.35 :
  - pilotes I2C EASE regroupés dans EASE_Hardware.h ;
  - variables inutilisées retirées et états capteurs compactés ;
  - buffer de commandes réduit ;
  - journal CSV autonome, avec nom et localisation sur chaque ligne ;
  - écriture SD ligne par ligne, sans grand buffer CSV ;
  - PEND.DAT binaire conservé pour une reprise sans perte ;
  - premier id_ligne d'un journal vierge = 1 ;
  - nom court de journal dérivé du nom appareil et rotation par semaine, mois ou taille ;
  - valeurs capteurs visibles pendant CHAUFFE ;
  - valeurs visibles pendant CHAUFFE, sans masquer le statut apres reset USB ;
  - miroir de configuration EASE.CFG sur SD ;
  - mesure du temps I/O de PEND.DAT et du journal ;
  - remise a zero volontaire des journaux et des IDs avec confirmation explicite.
  - validation de l'integrite RTC avant horodatage CSV ;
  - prefixe de fichier 8.3 identique a celui affiche par la page Web.
  - horodatage CSV conservé même si le drapeau OS RTC est présent.
  - mise a l’heure RTC : STOP libéré puis écriture atomique des 7 registres.

  Les bibliothèques Arduino Wire, SD, SPI, EEPROM et SoftwareSerial restent
  séparées : les fusionner ne réduirait pas leur RAM/Flash et augmenterait le
  risque de divergence avec le core Arduino.
*/
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <EEPROM.h>
#define _SS_MAX_RX_BUFF 16
#include <SoftwareSerial.h>
#include <string.h>
#include "EASE_Hardware.h"

const uint8_t SD_CS_PIN=4, CO2_RX_PIN=2, CO2_TX_PIN=3, HCHO_PIN=A0, ENC_A=5, ENC_B=6;
const uint8_t SI7021_ADDR=0x40, RTC_ADDR=0x51, LCD_ADDR=0x3E, RGB_ADDR=0x30;
const uint32_t USB_BAUD=115200UL;
// Les délais démarrent à chaque reset de l'Uno. Une connexion Web Serial
// peut donc afficher des valeurs encore utiles, mais leur statut reste CHAUFFE.
const unsigned long CO2_WARMUP_MS=180000UL;
const unsigned long LCD_REFRESH_MS=1000UL, LCD_PAGE_MS=4000UL, ENC_HOLD_MS=15000UL;
const unsigned long SAMPLE_MS=5000UL;
const uint32_t ROTATE_BYTES=4UL*1024UL*1024UL;
const uint16_t LAST_SLOT=999, ID_BLOCK=256;
const uint16_t SALVE_GUARD_BYTES=2600; // 11 lignes avec nom + coordonnées + PM
const char PEND_FILE[]="PEND.DAT";
const char CFG_FILE[]="CONFIG.CFG";
const uint16_t PEND_MAGIC=0xE512;
const uint8_t CFG_HAS_POS=0x01, CFG_ROT_WEEK=0x02, CFG_ROT_MONTH=0x04, CFG_FILE_NEW=0x80;
const uint8_t ROT_SIZE=0, ROT_WEEK=1, ROT_MONTH=2;
const int PERIOD_ADDR=8;

const uint8_t S_CO2=0x01, S_AIR=0x02, S_HCHO=0x04, S_SGP=0x08, S_BARO=0x20, S_GAS=0x40, S_PM=0x80;
const uint8_t CO2_CMD[9] PROGMEM={0xFF,0x01,0x86,0,0,0,0,0,0x79};
SoftwareSerial co2Serial(CO2_RX_PIN,CO2_TX_PIN);

struct Measurement {
  int co2, tair, hum, hcho, baro, tvoc, no2, co, pm1, pm25, pm10;
  uint8_t valid;
};
struct RtcTime { uint8_t yy,mo,dd,hh,mm,ss; };
struct Persist {
  uint16_t magic;
  uint16_t interval;
  uint16_t hchoR0;
  int32_t latE6;
  int32_t lonE6;
  char name[21];
  uint16_t activeSlot;
  uint8_t locationSet;
  uint16_t check;
} __attribute__((packed));
struct IdStore { uint32_t next; uint16_t check; } __attribute__((packed));
struct Pending {
  uint16_t magic;
  uint8_t version;
  uint16_t slot;
  uint32_t firstId;
  RtcTime t;
  int co2,tair,hum,hcho,baro,tvoc,no2,co,pm1,pm25,pm10;
  uint8_t valid;
  uint8_t warm;
  uint16_t check;
} __attribute__((packed));

Persist cfg={0xEA53,60,3428,0,0,"EASE-UNO",0,CFG_ROT_WEEK,0};
Measurement lastM;
// Structure globale : elle évite d'empiler une copie de Pending pendant les
// fonctions SD. La reprise est volontairement at-least-once : après un incident,
// la salve temporaire est réécrite intégralement plutôt que partiellement perdue.
Pending pending;
bool pendingLoaded=false;
uint32_t lineNext=1,activeBytes=0;
unsigned long bootMs,lastSampleMs,lastLogMs,lastLcdMs,lastPageMs,lastSdWriteMs;
uint16_t lastPendIoMs=0,lastLogIoMs=0;
// États regroupés pour limiter les octets statiques.
const uint8_t RD_BARO=0x01, RD_SGP=0x02, RD_GAS=0x04, RD_PM=0x08;
uint8_t readyFlags=0;
bool sdMounted=false,sdWriteOk=false;
uint8_t sdErr=0,lcdPage=0,encLast=0;
unsigned long encUntil=0,encTick=0;
// 32 octets couvrent la commande la plus longue : SET_NAME + 20 caracteres.
char cmd[30]; uint8_t cmdLen=0;

// Tables en mémoire programme : elles évitent de dupliquer les longues chaînes
// de libellés dans le code LCD et CSV.
const char N_CO2[] PROGMEM="co2_ppm";
const char N_TAIR[] PROGMEM="temperature_air_c";
const char N_HUM[] PROGMEM="humidite_pct";
const char N_HCHO[] PROGMEM="hcho_ppm_est";
const char N_BARO[] PROGMEM="pression_hpa";
const char N_TVOC[] PROGMEM="tvoc_ppb";
const char N_NO2[] PROGMEM="gas_no2_index";
const char N_CO[] PROGMEM="gas_co_index";
const char N_PM1[] PROGMEM="pm1_ug_m3";
const char N_PM25[] PROGMEM="pm25_ug_m3";
const char N_PM10[] PROGMEM="pm10_ug_m3";
const char *const METRIC_NAME[] PROGMEM={N_CO2,N_TAIR,N_HUM,N_HCHO,N_BARO,N_TVOC,N_NO2,N_CO,N_PM1,N_PM25,N_PM10};
const uint8_t METRIC_BIT[] PROGMEM={S_CO2,S_AIR,S_AIR,S_HCHO,S_BARO,S_SGP,S_GAS,S_GAS,S_PM,S_PM,S_PM};
const uint16_t METRIC_X10_MASK=(1U<<1)|(1U<<2)|(1U<<3)|(1U<<4);

const char L_CO2[] PROGMEM="CO2 ";
const char L_AIR[] PROGMEM="AIR ";
const char L_HUM[] PROGMEM="HUM ";
const char L_HCHO[] PROGMEM="HCHO ";
const char L_PRES[] PROGMEM="PRES ";
const char L_TVOC[] PROGMEM="TVOC ";
const char L_NO2[] PROGMEM="NO2 ";
const char L_CO[] PROGMEM="CO ";
const char L_CH[] PROGMEM="CHAUFFE";
const char L_ERR[] PROGMEM="ERR";
const char U_C[] PROGMEM=" C";
const char U_PCT[] PROGMEM=" %";
const char U_PPM[] PROGMEM=" ppm";
const char U_HPA[] PROGMEM=" hPa";
const char U_PPB[] PROGMEM=" ppb";
const char U_IDX[] PROGMEM=" idx";
const char *const LCD_LABEL[] PROGMEM={L_CO2,L_AIR,L_HUM,L_HCHO,L_PRES,L_TVOC,L_NO2,L_CO};
const char *const LCD_UNIT[] PROGMEM={U_PPM,U_C,U_PCT,U_PPM,U_HPA,U_PPB,U_IDX,U_IDX};
const uint8_t LCD_METRIC[] PROGMEM={0,1,2,3,4,5,6,7};
const uint8_t LCD_PAGE_A[] PROGMEM={1,0,4,6};
const uint8_t LCD_PAGE_B[] PROGMEM={2,3,5,7};

// Commandes Web Serial en mémoire programme : elles ne doivent pas occuper
// la SRAM limitée de l'ATmega328P.
const char C_PING[] PROGMEM="P";
const char C_STATUS[] PROGMEM="S";
const char C_GET_CFG[] PROGMEM="G";
const char C_SET_NAME[] PROGMEM="N ";
const char C_SET_POS[] PROGMEM="P ";
const char C_SET_INTERVAL[] PROGMEM="I ";
const char C_SET_ROTATE[] PROGMEM="R ";
const char C_SET_RTC[] PROGMEM="T ";
const char C_SAVE_CFG[] PROGMEM="V";
const char C_RESET_DATA[] PROGMEM="X 1";

// -------- utilitaires --------
void clearM(Measurement&m){m.co2=m.tair=m.hum=m.hcho=m.baro=m.tvoc=m.no2=m.co=m.pm1=m.pm25=m.pm10=-1;m.valid=0;}
uint16_t __attribute__((noinline)) sum16(const uint8_t*p,uint8_t n){uint16_t s=0;while(n--)s=(uint16_t)(s+*p++);return s;}
uint16_t cfgSum(const Persist&p){return sum16((const uint8_t*)&p,(uint8_t)(sizeof(Persist)-2));}
uint16_t idSum(const IdStore&p){return sum16((const uint8_t*)&p,4);}
uint16_t pendSum(const Pending&p){return sum16((const uint8_t*)&p,(uint8_t)(sizeof(Pending)-2));}
void sanitize(char*s){uint8_t w=0;for(uint8_t r=0;s[r]&&w<20;r++){char c=s[r];bool ok=(c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='-'||c=='_'||c==' '||c=='.';s[w++]=ok?c:'-';}while(w&&s[w-1]==' ')w--;if(!w){s[0]='E';s[1]='A';s[2]='S';s[3]='E';s[4]='-';s[5]='U';s[6]='N';s[7]='O';s[8]=0;return;}s[w]=0;}
void csvX10(File&f,int x){if(x<0){f.write('-');x=-x;}f.print(x/10);f.write(',');f.print(x%10);}
uint8_t rotationMode(){return(cfg.locationSet&CFG_ROT_MONTH)?ROT_MONTH:((cfg.locationSet&CFG_ROT_WEEK)?ROT_WEEK:ROT_SIZE);}
char rotationCode(){uint8_t r=rotationMode();return r==ROT_MONTH?'M':(r==ROT_WEEK?'W':'S');}
void savePeriod(uint16_t key){EEPROM.put(PERIOD_ADDR,key);}
void clearPeriod(){uint16_t key=0xFFFFU;EEPROM.put(PERIOD_ADDR,key);}

// -------- EEPROM / IDs --------
const int ID_ADDR=0, CFG_ADDR=32;
void saveCfg(){cfg.magic=0xEA53;cfg.check=cfgSum(cfg);EEPROM.put(CFG_ADDR,cfg);}
bool reserveIds(){IdStore s;EEPROM.get(ID_ADDR,s);if(s.next==0||s.next==0xFFFFFFFFUL||s.check!=idSum(s))s.next=1;if(s.next>0xFFFFFFFFUL-ID_BLOCK)return false;lineNext=s.next;s.next=s.next+ID_BLOCK;s.check=idSum(s);EEPROM.put(ID_ADDR,s);return true;}
void loadCfg(){Persist p;EEPROM.get(CFG_ADDR,p);if(p.magic==0xEA53&&p.check==cfgSum(p)&&p.interval>=10&&p.interval<=3600&&p.hchoR0>10&&p.activeSlot<=LAST_SLOT){cfg=p;sanitize(cfg.name);}else{sanitize(cfg.name);saveCfg();}reserveIds();}

// -------- RTC --------
uint8_t bcd(uint8_t v){return(uint8_t)((v>>4)*10+(v&15));}
uint8_t rtcDays(uint8_t yy,uint8_t mo){return mo==2?((yy&3)?28:29):((mo==4||mo==6||mo==9||mo==11)?30:31);}
bool rtcValid(const RtcTime&t){return t.mo>0&&t.mo<13&&t.dd>0&&t.dd<=rtcDays(t.yy,t.mo)&&t.hh<24&&t.mm<60&&t.ss<60;}
bool __attribute__((noinline)) rtcRead(RtcTime&t){
  muxDeselect();Wire.beginTransmission(RTC_ADDR);Wire.write((uint8_t)0x04);
  if(Wire.endTransmission(false)!=0||Wire.requestFrom((int)RTC_ADDR,7)!=7)return false;
  uint8_t b[7];for(uint8_t i=0;i<7;i++){if(!Wire.available())return false;b[i]=Wire.read();}
  // OS indique que l'historique de l'horloge n'est pas garanti. Il ne rend pas
  // les 7 registres illisibles : les supprimer du CSV masquait donc inutilement
  // la date et l'heure. La validité de calendrier reste contrôlée ci-dessous.
  t.ss=bcd(b[0]&0x7F);t.mm=bcd(b[1]&0x7F);t.hh=bcd(b[2]&0x3F);t.dd=bcd(b[3]&0x3F);t.mo=bcd(b[5]&0x1F);t.yy=bcd(b[6]);
  return rtcValid(t);
}
void __attribute__((noinline)) writeDateTime(File&f,const RtcTime&t){
  if(!t.mo){f.write(';');return;}
  char b[17]={'2','0',0,0,0,0,0,0,';',0,0,':',0,0,':',0,0};
  const uint8_t v[6]={t.yy,t.mo,t.dd,t.hh,t.mm,t.ss};uint8_t p=2;
  for(uint8_t i=0;i<3;i++,p+=2){b[p]=(char)('0'+v[i]/10);b[p+1]=(char)('0'+v[i]%10);}p=9;
  for(uint8_t i=3;i<6;i++,p+=3){b[p]=(char)('0'+v[i]/10);b[p+1]=(char)('0'+v[i]%10);}
  f.write((const uint8_t*)b,17);
}
uint8_t toBcd(uint8_t v){return(uint8_t)(((v/10)<<4)|(v%10));}
uint8_t pair2(const char*s){return(uint8_t)((s[0]-'0')*10+(s[1]-'0'));}
bool rtcSetCompact(const char*s){
  if(s[0]!='2'||s[1]!='0')return false;
  for(uint8_t i=2;i<14;i++)if(s[i]<'0'||s[i]>'9')return false;
  if(s[14])return false;
  RtcTime t={pair2(s+2),pair2(s+4),pair2(s+6),pair2(s+8),pair2(s+10),pair2(s+12)};
  if(!rtcValid(t))return false;
  // Le premier octet (secondes) porte OS. L'écriture de 0 dans ce bit le
  // réarme lorsque l'oscillateur fonctionne. Les 7 registres sont écrits
  // ensemble, comme le demande le PCF85063TP.
  uint8_t b[7]={toBcd(t.ss),toBcd(t.mm),toBcd(t.hh),toBcd(t.dd),0,toBcd(t.mo),toBcd(t.yy)};
  muxDeselect();
  // Libère STOP et force le mode 24 h avant l'écriture atomique de la date.
  Wire.beginTransmission(RTC_ADDR);Wire.write((uint8_t)0);Wire.write((uint8_t)0);
  if(Wire.endTransmission())return false;
  Wire.beginTransmission(RTC_ADDR);Wire.write((uint8_t)4);Wire.write(b,7);
  return Wire.endTransmission()==0;
}
const uint16_t MONTH_DAYS[] PROGMEM={0,31,59,90,120,151,181,212,243,273,304,334};
uint16_t dayIndex(const RtcTime&t){uint16_t d=(uint16_t)365*t.yy+(t.yy+3)/4+pgm_read_word(&MONTH_DAYS[t.mo-1])+t.dd-1;if(t.mo>2&&!(t.yy&3))d++;return d;}
bool calendarDue(uint16_t&key){
  if(rotationMode()==ROT_SIZE){key=0;return false;}
  RtcTime t;if(!rtcRead(t)){key=0;return false;}
  key=rotationMode()==ROT_MONTH?(uint16_t)(t.yy*100U+t.mo):(uint16_t)((dayIndex(t)+5U)/7U);
  uint16_t old;EEPROM.get(PERIOD_ADDR,old);
  if(old==0xFFFFU){savePeriod(key);return false;}
  return old!=key;
}

// -------- LCD + encodeur --------
// Grove LCD RGB Backlight v5.0 : le test autonome a confirmé les adresses
// texte 0x3E et RGB 0x30. Cette séquence reprend celle qui fonctionne avec V5.
const uint8_t LCD_PAGE_COUNT=6;
uint8_t warmMask();
void lcdTx(uint8_t c,uint8_t v){Wire.beginTransmission(LCD_ADDR);Wire.write(c);Wire.write(v);Wire.endTransmission();}
void lcdCmd(uint8_t v){lcdTx(0x80,v);} void lcdData(uint8_t v){lcdTx(0x40,v);}
void rgb(uint8_t r,uint8_t v){Wire.beginTransmission(RGB_ADDR);Wire.write(r);Wire.write(v);Wire.endTransmission();}
void lcdBegin(){
  muxDeselect();delay(50);
  lcdCmd(0x28);delay(5);lcdCmd(0x28);delayMicroseconds(150);lcdCmd(0x28);lcdCmd(0x28);
  lcdCmd(0x0C);lcdCmd(0x01);delay(2);lcdCmd(0x06);
  rgb(0,0x07);delay(1);rgb(4,0x15);  // configuration RGB du Grove LCD v5.0
  rgb(6,0);rgb(7,100);rgb(8,200);    // bleu doux, lisible
}
void fill(char*b){for(uint8_t i=0;i<16;i++)b[i]=' ';b[16]=0;}
void __attribute__((noinline)) uput(char*b,uint8_t&p,unsigned v){unsigned div=10000;bool started=false;while(div>1){unsigned d=v/div;if(d||started){if(p<16)b[p++]=(char)('0'+d);started=true;}v%=div;div/=10;}if(p<16)b[p++]=(char)('0'+v);}
void __attribute__((noinline)) dput(char*b,uint8_t&p,int v){if(v<0){if(p<16)b[p++]='-';v=-v;}uput(b,p,(unsigned)v/10);if(p<15)b[p++]='.';if(p<16)b[p++]='0'+v%10;}
void id4(char*b,uint8_t&p,uint32_t id){id%=10000UL;for(uint16_t div=1000;div;div/=10)if(p<16)b[p++]=(char)('0'+(id/div)%10);}
void lcdLine(uint8_t row,const char*b){lcdCmd(row?0xC0:0x80);for(uint8_t i=0;i<16;i++)lcdData(b[i]?b[i]:' ');}
void lcdPuts(char*b,uint8_t&p,const char*s){char c;while(p<16&&(c=(char)pgm_read_byte(s++)))b[p++]=c;}
void lcdMetric(char*b,uint8_t&p,uint8_t item,uint8_t warm){
  const char* label=(const char*)pgm_read_word(&LCD_LABEL[item]);
  const char* unit=(const char*)pgm_read_word(&LCD_UNIT[item]);
  const uint8_t mi=pgm_read_byte(&LCD_METRIC[item]);
  const uint8_t bit=pgm_read_byte(&METRIC_BIT[mi]);
  lcdPuts(b,p,label);
  if(lastM.valid&bit){
    int v=((const int*)&lastM)[mi];
    if(METRIC_X10_MASK&(1U<<mi))dput(b,p,v);else uput(b,p,(unsigned)v);
    if(warm&bit){if(p<16)b[p++]=' ';if(p<16)b[p++]='C';}
    else lcdPuts(b,p,unit);
  }else if(warm&bit)lcdPuts(b,p,L_CH);
  else lcdPuts(b,p,L_ERR);
}
void lcdRefresh(){
  muxDeselect();char a[17],b[17];fill(a);fill(b);uint8_t p=0,w=warmMask();
  if(lcdPage<4){
    lcdMetric(a,p,pgm_read_byte(&LCD_PAGE_A[lcdPage]),w);
    p=0;lcdMetric(b,p,pgm_read_byte(&LCD_PAGE_B[lcdPage]),w);
  }else if(lcdPage==4){
    if(lastM.valid&S_PM){
      a[p++]='P';a[p++]='M';a[p++]='1';a[p++]=':';uput(a,p,lastM.pm1);a[p++]=' ';a[p++]='P';a[p++]='2';a[p++]='5';a[p++]=':';uput(a,p,lastM.pm25);
      p=0;b[p++]='P';b[p++]='M';b[p++]='1';b[p++]='0';b[p++]=':';uput(b,p,lastM.pm10);
      if(w&S_PM){b[p++]=' ';b[p++]='C';}else{b[p++]=' ';b[p++]='u';b[p++]='g';b[p++]='/';b[p++]='m';b[p++]='3';}
    }else if(w&S_PM){lcdPuts(a,p,L_CH);b[p++]='P';b[p++]='M';b[p++]=' ';b[p++]='3';b[p++]='0';b[p++]='s';}
    else lcdPuts(a,p,L_ERR);
  }else{
    a[p++]='S';a[p++]='D';a[p++]=':';if(!sdMounted){a[p++]='A';a[p++]='B';a[p++]='S';a[p++]='E';a[p++]='N';a[p++]='T';}else if(sdErr){a[p++]='E';uput(a,p,sdErr);}else if(sdWriteOk){a[p++]='O';a[p++]='K';}else{a[p++]='P';a[p++]='R';a[p++]='E';a[p++]='T';}
    p=0;b[p++]='I';b[p++]='D';b[p++]=':';id4(b,p,lineNext);if(p<16)b[p++]=' ';b[p++]='L';b[p++]='O';b[p++]='G';b[p++]=':';if(sdErr){b[p++]='E';uput(b,p,sdErr);}else if(sdWriteOk){b[p++]='O';b[p++]='K';}else b[p++]='-';
  }
  lcdLine(0,a);lcdLine(1,b);
}
void encInit(){pinMode(ENC_A,INPUT_PULLUP);pinMode(ENC_B,INPUT_PULLUP);encLast=(uint8_t)((digitalRead(ENC_A)<<1)|digitalRead(ENC_B));}
void encPoll(){uint8_t s=(uint8_t)((digitalRead(ENC_A)<<1)|digitalRead(ENC_B));if(s==encLast)return;unsigned long n=millis();if(n-encTick<5)return;uint8_t q=(uint8_t)((encLast<<2)|s);int8_t d=0;if(q==1||q==7||q==14||q==8)d=1;else if(q==2||q==11||q==13||q==4)d=-1;encLast=s;encTick=n;if(!d)return;int8_t x=(int8_t)lcdPage+d;if(x<0)x=LCD_PAGE_COUNT-1;else if(x>=LCD_PAGE_COUNT)x=0;lcdPage=(uint8_t)x;encUntil=n+ENC_HOLD_MS;lastLcdMs=0;}

// -------- capteurs --------
bool readCo2(Measurement&m){while(co2Serial.available())co2Serial.read();for(uint8_t i=0;i<9;i++)co2Serial.write(pgm_read_byte(&CO2_CMD[i]));delay(10);uint8_t b[9],n=0;unsigned long s=millis();while(n<9&&millis()-s<1000UL)if(co2Serial.available())b[n++]=(uint8_t)co2Serial.read();if(n!=9)return false;uint16_t z=0;for(uint8_t i=1;i<8;i++)z+=b[i];if((uint8_t)(1+(0xFF^(uint8_t)z))!=b[8])return false;m.co2=(int)b[2]*256+b[3];m.valid|=S_CO2;return true;}
bool siRaw(uint8_t c,uint16_t&r,uint16_t wait){
#if SI7021_ON_MUX
  if(!muxSelectFor(MUX_CH_SI7021))return false;
#else
  // Cette branche est reservee a un bus I2C physiquement separe. Sur le bus
  // Uno courant, SI7021_ON_MUX doit rester a 1 (conflit d'adresse 0x40).
  muxDeselect();
#endif
  delay(2);Wire.beginTransmission(SI7021_ADDR);Wire.write(c);if(Wire.endTransmission()!=0)return false;delay(wait);uint8_t b[3],i=0,n=Wire.requestFrom((int)SI7021_ADDR,3);while(Wire.available()&&i<3)b[i++]=Wire.read();while(Wire.available())Wire.read();if(n!=3||i!=3||easeCrc31(b,2,0)!=b[2])return false;r=((uint16_t)b[0]<<8)|b[1];return true;}
bool readAir(Measurement&m){uint16_t t,h;if(!siRaw(0xF3,t,85)||!siRaw(0xF5,h,50))return false;t&=0xFFFC;h&=0xFFFC;int32_t tc=-4685L+17572L*(int32_t)t/65536L,hc=-600L+12500L*(int32_t)h/65536L;if(tc<-4000||tc>12500||hc<0||hc>10000)return false;m.tair=(int)((tc+(tc>=0?5:-5))/10);m.hum=(int)((hc+5)/10);m.valid|=S_AIR;return true;}
const uint16_t H_R[32] PROGMEM={10,12,13,16,18,21,24,28,32,37,43,50,58,67,77,90,104,120,139,161,186,215,249,289,334,387,448,518,599,694,803,930};
const uint16_t H_P[32] PROGMEM={1000,1000,973,718,530,391,288,213,157,116,85,63,46,34,25,19,14,10,7,6,4,3,2,2,1,1,1,0,0,0,0,0};
uint16_t hAdc(){uint32_t s=0;uint16_t lo=1023,hi=0;for(uint8_t i=0;i<7;i++){uint16_t v=analogRead(HCHO_PIN);s+=v;if(v<lo)lo=v;if(v>hi)hi=v;delay(2);}return(uint16_t)((s-lo-hi+2)/5);}
bool readHcho(Measurement&m){uint16_t a=hAdc();if(a==0||a>=1022)return false;uint32_t rs=(102300UL+a/2)/a;if(rs<=100)return false;rs-=100;uint16_t r=(uint16_t)(rs*100UL/cfg.hchoR0);uint8_t i=0;while(i<31&&r>=pgm_read_word(&H_R[i+1]))i++;m.hcho=pgm_read_word(&H_P[i]);m.valid|=S_HCHO;return true;}
void updateSensors(){clearM(lastM);readCo2(lastM);readAir(lastM);readHcho(lastM);unsigned long now=millis();if(readyFlags&RD_BARO){int p;if(readDps310(p)){lastM.baro=p;lastM.valid|=S_BARO;}}if(readyFlags&RD_SGP){int v;if(readSgp30(v,now)){lastM.tvoc=v;lastM.valid|=S_SGP;}}if(readyFlags&RD_GAS){int n,c;if(readGasV2(n,c,now)){lastM.no2=n;lastM.co=c;lastM.valid|=S_GAS;}}if(readyFlags&RD_PM){int a,b,c;if(readHm3301(a,b,c,now)){lastM.pm1=a;lastM.pm25=b;lastM.pm10=c;lastM.valid|=S_PM;}}}
bool initAll(){bool airOk=false;
#if SI7021_ON_MUX
  if(muxSelectFor(MUX_CH_SI7021)){
#else
  muxDeselect();{
#endif
    Wire.beginTransmission(SI7021_ADDR);airOk=Wire.endTransmission()==0;
  }
  readyFlags=0;if(initDps310())readyFlags|=RD_BARO;if(initSgp30())readyFlags|=RD_SGP;if(initGasV2())readyFlags|=RD_GAS;if(initHm3301(bootMs))readyFlags|=RD_PM;return airOk;}
uint8_t __attribute__((noinline)) warmMask(){
  uint8_t w=0;unsigned long now=millis();
  if(now-bootMs<CO2_WARMUP_MS)w|=S_CO2;
  if(now-bootMs<SGP30_WARMUP_MS)w|=S_SGP;
  if(now-bootMs<GASV2_WARMUP_MS)w|=S_GAS;
  if((readyFlags&RD_PM)&&!hm3301WarmedUp(now))w|=S_PM;
  return w;
}

// -------- SD : écriture et reprise PEND.DAT --------
// Les File sont construits directement avec SD.open(). Certaines variantes de
// la bibliotheque SD ne conservent pas de maniere fiable un descripteur obtenu
// par affectation (f = SD.open(...)). La reprise PEND.DAT ouvre EASE.CSV une
// seule fois : entete eventuel + les 11 lignes de la salve dans la meme session.
void sdPrep(){pinMode(SD_CS_PIN,OUTPUT);pinMode(10,OUTPUT);digitalWrite(SD_CS_PIN,HIGH);digitalWrite(10,HIGH);SPI.begin();}
bool __attribute__((noinline)) mountSD(){if(!SD.begin(SD_CS_PIN)){sdMounted=false;sdWriteOk=false;sdErr=1;return false;}sdMounted=true;return true;}
void __attribute__((noinline)) logName(char*n,uint16_t slot){
  // Exactement la meme regle que la page Web : les deux premiers caracteres
  // alphanumeriques, en majuscules. Le nom reste donc FAT 8.3 et predictable.
  uint8_t k=0;for(uint8_t i=0;cfg.name[i]&&k<2;i++){char c=cfg.name[i];
    if(c>='a'&&c<='z')c-='a'-'A';
    if((c>='A'&&c<='Z')||(c>='0'&&c<='9'))n[k++]=c;
  }while(k<2)n[k++]='X';
  n[2]='0'+(slot/100)%10;n[3]='0'+(slot/10)%10;n[4]='0'+slot%10;n[5]='.';n[6]='C';n[7]='S';n[8]='V';n[9]=0;
}
void __attribute__((noinline)) printCoordE6(Print& out,int32_t e6){
  if(e6<0){out.write('-');e6=-e6;}
  uint32_t v=(uint32_t)e6;out.print(v/1000000UL);out.write(',');v%=1000000UL;
  uint32_t d=100000UL;while(d){out.write((uint8_t)('0'+v/d));v%=d;d/=10UL;}
}
void __attribute__((noinline)) csvHeader(File&f){
  f.write((uint8_t)0xEF);f.write((uint8_t)0xBB);f.write((uint8_t)0xBF);
  f.println(F("id_ligne;date;heure;nom;latitude;longitude;mesure;valeur;etat"));
}
bool ensureLog(const char*n,uint32_t&size){
  if(!sdMounted&&!mountSD())return false;
  File f=SD.open(n,FILE_WRITE);
  if(!f){sdMounted=false;sdErr=2;sdWriteOk=false;return false;}
  if(f.size()==0){csvHeader(f);}
  size=f.size();f.close();
  if(size<40UL){sdErr=2;sdWriteOk=false;return false;}
  return true;
}
bool activateCurrent(){char n[10];logName(n,cfg.activeSlot);return ensureLog(n,activeBytes);}
void resetIdsForEmptyJournal();
// CONFIG.CFG est un miroir humainement lisible. La configuration de reference
// reste en EEPROM : une coupure pendant la recreation du fichier SD n'empeche
// donc jamais le logger de demarrer.
bool syncConfigFile(){
  if(!sdMounted&&!mountSD())return false;
  if(SD.exists(CFG_FILE)&&!SD.remove(CFG_FILE)){sdErr=8;return false;}
  File f=SD.open(CFG_FILE,FILE_WRITE);
  if(!f){sdErr=8;sdWriteOk=false;return false;}
  // Une ligne compacte : nom;intervalle;W|M|S;latitude;longitude.
  f.print(cfg.name);f.write(';');f.print(cfg.interval);f.write(';');f.write(rotationCode());f.write(';');
  if(cfg.locationSet&CFG_HAS_POS){printCoordE6(f,cfg.latE6);f.write(';');printCoordE6(f,cfg.lonE6);}f.println();
  uint32_t size=f.size();f.close();
  if(size<6UL){sdErr=8;sdWriteOk=false;return false;}
  return true;
}
// Suppression volontaire des journaux et remise du compteur a ID 1. Cette
// action conserve les reglages EEPROM et EASE.CFG, mais efface PEND.DAT,
// EASE.CSV et E001.CSV..E999.CSV. Elle n'est appelee qu'apres confirmation
// explicite depuis l'interface Web Serial.
bool resetDataFiles(){
  if(!sdMounted&&!mountSD())return false;
  if(SD.exists(PEND_FILE)&&!SD.remove(PEND_FILE)){sdErr=5;return false;}
  char n[10];
  for(uint16_t slot=0;slot<=LAST_SLOT;slot++){
    logName(n,slot);
    if(SD.exists(n)&&!SD.remove(n)){sdErr=8;sdWriteOk=false;return false;}
  }
  cfg.activeSlot=0;activeBytes=0;pendingLoaded=false;
  IdStore st;st.next=1;st.check=idSum(st);EEPROM.put(ID_ADDR,st);
  lineNext=1;clearPeriod();
  if(!reserveIds()){sdErr=6;return false;}
  saveCfg();sdErr=0;sdWriteOk=false;lastSdWriteMs=0;
  if(!activateCurrent())return false;
  resetIdsForEmptyJournal();
  if(!syncConfigFile())return false;
  sdErr=0;return true;
}
// Sur un journal vierge, le premier ID est toujours 1. Après ce point, les
// blocs réservés peuvent créer des trous après coupure, jamais des doublons.
void resetIdsForEmptyJournal(){
  if(cfg.activeSlot!=0||SD.exists(PEND_FILE)||activeBytes>100UL)return;
  IdStore st;st.next=1;st.check=idSum(st);EEPROM.put(ID_ADDR,st);reserveIds();
}
bool rotateTarget(){
  uint16_t slot=cfg.activeSlot;char n[10];
  do{if(slot>=LAST_SLOT){sdErr=7;return false;}slot++;logName(n,slot);}while(SD.exists(n));
  uint32_t z=0;if(!ensureLog(n,z)){sdErr=7;return false;}cfg.activeSlot=slot;activeBytes=z;return true;
}
bool pickTarget(){
  uint16_t key=0;bool due=calendarDue(key),fresh=(cfg.locationSet&CFG_FILE_NEW)!=0;
  if(!due&&!fresh&&activeBytes+SALVE_GUARD_BYTES<=ROTATE_BYTES)return true;
  if(!rotateTarget())return false;
  cfg.locationSet&=(uint8_t)~CFG_FILE_NEW;if((due||fresh)&&key)savePeriod(key);saveCfg();return true;
}
void pendFrom(const Measurement&m,uint32_t firstId){
  pending.magic=PEND_MAGIC;pending.version=2;pending.slot=cfg.activeSlot;pending.firstId=firstId;
  if(!rtcRead(pending.t)){pending.t.yy=0;pending.t.mo=0;pending.t.dd=0;pending.t.hh=pending.t.mm=pending.t.ss=0;}
  pending.co2=m.co2;pending.tair=m.tair;pending.hum=m.hum;pending.hcho=m.hcho;
  pending.baro=m.baro;pending.tvoc=m.tvoc;pending.no2=m.no2;pending.co=m.co;pending.pm1=m.pm1;pending.pm25=m.pm25;pending.pm10=m.pm10;
  pending.valid=m.valid;pending.warm=warmMask();pending.check=pendSum(pending);
  pendingLoaded=true;
}
bool savePending(){
  if(SD.exists(PEND_FILE)){sdErr=5;return false;}
  if(!sdMounted&&!mountSD())return false;
  unsigned long started=millis();
  File f=SD.open(PEND_FILE,FILE_WRITE);
  if(!f){sdMounted=false;sdErr=3;sdWriteOk=false;return false;}
  size_t wrote=f.write((const uint8_t*)&pending,sizeof(Pending));uint32_t size=f.size();f.close();
  lastPendIoMs=(uint16_t)(millis()-started);
  if(wrote!=sizeof(Pending)||size<sizeof(Pending)){sdErr=3;sdWriteOk=false;return false;}
  return true;
}
bool loadPending(){
  if(!sdMounted&&!mountSD())return false;
  File f=SD.open(PEND_FILE,FILE_READ);
  if(!f){sdMounted=false;sdErr=3;sdWriteOk=false;return false;}
  int got=f.read((uint8_t*)&pending,sizeof(Pending));f.close();
  if(got!=(int)sizeof(Pending)||pending.magic!=PEND_MAGIC||pending.version!=2||pending.slot>LAST_SLOT||pending.check!=pendSum(pending)){
    sdErr=5;sdWriteOk=false;return false;
  }
  pendingLoaded=true;
  return true;
}
void writeState(File&f,bool ok,uint8_t bit,uint8_t warm){f.println((warm&bit)?F("CHAUFFE"):(ok?F("OK"):F("ERREUR")));}
void rowPrefix(File&f,uint32_t id,const Pending&p){
  f.print(id);f.write(';');writeDateTime(f,p.t);f.write(';');f.print(cfg.name);f.write(';');
  if(cfg.locationSet&CFG_HAS_POS)printCoordE6(f,cfg.latE6);f.write(';');if(cfg.locationSet&CFG_HAS_POS)printCoordE6(f,cfg.lonE6);f.write(';');
}
void writeRow(File&f,const Pending&p,uint8_t row){
  const uint8_t bit=pgm_read_byte(&METRIC_BIT[row]);
  const bool ok=(p.valid&bit)!=0;
  const char* name=(const char*)pgm_read_word(&METRIC_NAME[row]);
  const int v=((const int*)&p.co2)[row];
  rowPrefix(f,p.firstId+(uint32_t)row,p);f.print((const __FlashStringHelper*)name);f.write(';');
  if(ok){if(METRIC_X10_MASK&(1U<<row))csvX10(f,v);else f.print(v);}f.write(';');writeState(f,ok,bit,p.warm);
}
// Une seule ouverture de EASE.CSV : le journal est initialise si necessaire,
// puis les 11 lignes PEND.DAT sont ecrites avant fermeture. Ainsi la reprise
// ne depend plus d'une succession ouverture/fermeture de ce fichier.
bool appendPending(){
  char n[10];logName(n,pending.slot);
  if(!sdMounted&&!mountSD())return false;
  unsigned long started=millis();
  File f=SD.open(n,FILE_WRITE);
  if(!f){sdMounted=false;sdErr=2;sdWriteOk=false;return false;}
  uint32_t before=f.size();
  if(before==0){csvHeader(f);before=f.size();}
  for(uint8_t row=0;row<11;row++)writeRow(f,pending,row);
  uint32_t after=f.size();f.close();
  lastLogIoMs=(uint16_t)(millis()-started);
  if(after<=before){sdErr=4;sdWriteOk=false;return false;}
  activeBytes=after;cfg.activeSlot=pending.slot;
  return true;
}
bool recoverPending(){
  if(!SD.exists(PEND_FILE)){pendingLoaded=false;return true;}
  if(!pendingLoaded&&!loadPending())return false;
  if(!appendPending())return false;
  if(!SD.remove(PEND_FILE)){sdErr=5;sdWriteOk=false;return false;}
  pendingLoaded=false;sdErr=0;sdWriteOk=true;lastSdWriteMs=millis();
  return true;
}
bool logNow(){
  if(!sdMounted&&!mountSD())return false;
  if(!recoverPending())return false;
  if((uint8_t)(lineNext-1UL)>245U&&!reserveIds()){sdErr=6;return false;}
  if(!pickTarget())return false;
  pendFrom(lastM,lineNext);
  if(!savePending())return false;
  // L'ID est avancé dès que PEND.DAT est sécurisé, avant l'écriture des lignes.
  // Après une coupure : trous possibles, jamais réutilisation d'ID.
  lineNext+=11UL;
  if(!recoverPending())return false;
  return true;
}

// -------- protocole Web Serial compact --------
bool sameP(const char*a,const char*b){char c;while((c=(char)pgm_read_byte(b++)))if(*a++!=c)return false;return !*a;}
bool prefP(const char*a,const char*b){char c;while((c=(char)pgm_read_byte(b++)))if(*a++!=c)return false;return true;}
long parseLong(const char*&p,bool&ok){while(*p==' ')p++;bool neg=false;if(*p=='-'){neg=true;p++;}long v=0;ok=false;while(*p>='0'&&*p<='9'){v=v*10+(*p++-'0');ok=true;}return neg?-v:v;}
// Coordonnées reçues et retournées en degrés décimaux conventionnels. L'EEPROM
// conserve seulement la représentation E6 pour éviter les flottants sur Uno.
bool parseCoordE6(const char*&p,int32_t&out){
  while(*p==' ')p++;
  bool neg=false;if(*p=='-'||*p=='+'){neg=*p=='-';p++;}
  uint32_t whole=0;bool digit=false;
  while(*p>='0'&&*p<='9'){
    if(whole>2147UL)return false;
    whole=whole*10UL+(uint8_t)(*p++-'0');digit=true;
  }
  uint32_t frac=0;uint8_t decimals=0;
  if(*p==','||*p=='.'){
    p++;
    while(*p>='0'&&*p<='9'){
      if(decimals>=6)return false;
      frac=frac*10UL+(uint8_t)(*p++-'0');decimals++;digit=true;
    }
  }
  if(!digit)return false;
  while(decimals<6){frac*=10UL;decimals++;}
  uint32_t value=whole*1000000UL+frac;
  if(value>2147483647UL)return false;
  out=neg?-(int32_t)value:(int32_t)value;
  return true;
}
void sendCfg(){
  Serial.print(F("#C;"));Serial.print(cfg.interval);Serial.print(';');Serial.print(cfg.name);Serial.print(';');
  if(cfg.locationSet&CFG_HAS_POS)printCoordE6(Serial,cfg.latE6);Serial.print(';');
  if(cfg.locationSet&CFG_HAS_POS)printCoordE6(Serial,cfg.lonE6);Serial.print(';');Serial.print((cfg.locationSet&CFG_HAS_POS)?1:0);Serial.print(';');Serial.write(rotationCode());Serial.println();
}
void setName(const char*s){if(pendingLoaded){Serial.println(F("#ERR"));return;}while(*s==' ')s++;strncpy(cfg.name,s,20);cfg.name[20]=0;sanitize(cfg.name);cfg.locationSet|=CFG_FILE_NEW;Serial.println(F("#OK"));}
void setPos(const char*s){
  if(pendingLoaded){Serial.println(F("#ERR"));return;}
  int32_t lat,lon;if(!parseCoordE6(s,lat)||!parseCoordE6(s,lon)){Serial.println(F("#ERR"));return;}
  while(*s==' ')s++;
  if(*s||lat<-90000000L||lat>90000000L||lon<-180000000L||lon>180000000L){Serial.println(F("#ERR"));return;}
  cfg.latE6=lat;cfg.lonE6=lon;cfg.locationSet|=CFG_HAS_POS;Serial.println(F("#OK"));
}
void setInterval(const char*s){bool ok;long v=parseLong(s,ok);if(!ok||v<10||v>3600){Serial.println(F("#ERR"));return;}cfg.interval=(uint16_t)v;Serial.println(F("#OK"));}
void setRotate(const char*s){uint8_t m=0;if(*s=='W')m=CFG_ROT_WEEK;else if(*s=='M')m=CFG_ROT_MONTH;else if(*s!='S'){Serial.println(F("#ERR"));return;}cfg.locationSet=(cfg.locationSet&(uint8_t)~(CFG_ROT_WEEK|CFG_ROT_MONTH))|m;cfg.locationSet|=CFG_FILE_NEW;Serial.println(F("#OK"));}
void saveConfiguration(){saveCfg();if(syncConfigFile())Serial.println(F("#OK"));else Serial.println(F("#ERR"));}
void sendStatus(){
  Serial.print(F("#S"));const int* v=(const int*)&lastM;
  for(uint8_t i=0;i<11;i++){Serial.write(';');Serial.print(v[i]);}
  Serial.write(';');Serial.print(warmMask());Serial.write(';');Serial.print(sdMounted?1:0);Serial.write(';');Serial.print(sdErr);
  Serial.write(';');Serial.print(cfg.activeSlot);Serial.write(';');Serial.print(activeBytes);Serial.write(';');Serial.print(sdWriteOk?1:0);
  Serial.write(';');Serial.print(lastSdWriteMs?(millis()-lastSdWriteMs)/1000UL:0UL);
  Serial.write(';');Serial.print(lastPendIoMs);Serial.write(';');Serial.print(lastLogIoMs);
  Serial.println();
}
void command(const char*s){
  if(sameP(s,C_PING))Serial.println(F("PONG"));
  else if(sameP(s,C_STATUS))sendStatus();
  else if(sameP(s,C_GET_CFG))sendCfg();
  else if(prefP(s,C_SET_NAME))setName(s+2);
  else if(prefP(s,C_SET_POS))setPos(s+2);
  else if(prefP(s,C_SET_INTERVAL))setInterval(s+2);
  else if(prefP(s,C_SET_ROTATE))setRotate(s+2);
  else if(prefP(s,C_SET_RTC)){if(rtcSetCompact(s+2))Serial.println(F("#OK"));else Serial.println(F("#ERR"));}
  else if(sameP(s,C_SAVE_CFG))saveConfiguration();
  else if(sameP(s,C_RESET_DATA)){if(resetDataFiles())Serial.println(F("#OK"));else Serial.println(F("#ERR"));}
  else Serial.println(F("#ERR"));
}
void serialPoll(){while(Serial.available()){char c=(char)Serial.read();if(c=='\r'||c=='\n'){if(cmdLen){cmd[cmdLen]=0;command(cmd);cmdLen=0;}}else if(cmdLen<sizeof(cmd)-1)cmd[cmdLen++]=c;}}

void setup(){
  bootMs=millis();clearM(lastM);Serial.begin(USB_BAUD);co2Serial.begin(9600);Wire.begin();Wire.setClock(100000);sdPrep();loadCfg();initAll();encInit();lcdBegin();delay(120);
  if(mountSD()){
    if(recoverPending()){
      if(activateCurrent()){
        resetIdsForEmptyJournal();
        if(!SD.exists(CFG_FILE))syncConfigFile();
      }
    }else{sdWriteOk=false;}
  }
  lastSampleMs=0;lastLogMs=millis();lastLcdMs=0;lastPageMs=millis();Serial.println(F("#E43"));
}
void loop(){serialPoll();encPoll();if(readyFlags&RD_SGP)pollSgp30(millis());if(readyFlags&RD_GAS)pollGasV2(millis());if(readyFlags&RD_PM)pollHm3301(millis());if(millis()-lastSampleMs>=SAMPLE_MS){lastSampleMs=millis();updateSensors();}if(millis()-lastLcdMs>=LCD_REFRESH_MS){lastLcdMs=millis();lcdRefresh();}if(millis()-lastPageMs>=LCD_PAGE_MS&&millis()>=encUntil){lastPageMs=millis();lcdPage=(uint8_t)((lcdPage+1)%LCD_PAGE_COUNT);}if(millis()-lastLogMs>=(unsigned long)cfg.interval*1000UL){lastLogMs=millis();logNow();}}
