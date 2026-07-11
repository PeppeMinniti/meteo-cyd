// ============================================================================
//  CYD + BME280 - Stazione ambientale con UI "studio" e touch
//  File commentato a scopo didattico: ogni riga ha una spiegazione.
// ============================================================================
//
//  TEORIA - concetti chiave usati in questo programma
//  ---------------------------------------------------------------------------
//  1) SPRITE e FLICKER (sfarfallio)
//     Disegnare direttamente sullo schermo "pixel dopo pixel" fa vedere il
//     disegno mentre si forma: si cancella e ridisegna, e l'occhio percepisce
//     uno sfarfallio. Uno SPRITE e' un'immagine tenuta in RAM (un buffer fuori
//     schermo): si disegna tutto la' con calma e, quando e' pronto, lo si copia
//     sullo schermo in un colpo solo (pushSprite). Il display passa dal vecchio
//     fotogramma al nuovo gia' completo -> niente sfarfallio. Costo: RAM, pari a
//     larghezza x altezza x 2 byte (vedi RGB565).
//
//  2) RGB565 (il formato colore del display)
//     Ogni pixel e' un numero a 16 bit: 5 bit per il ROSSO, 6 per il VERDE, 5
//     per il BLU (in totale 16 = "565"). Il verde ha un bit in piu' perche'
//     l'occhio e' piu' sensibile alle sue sfumature. Da RGB a 0-255 si "comprime"
//     scartando i bit meno significativi (vedi la funzione rgb()). 16 bit/pixel
//     e' il motivo del "x 2 byte" nel calcolo della RAM degli sprite.
//
//  3) Perche' il TOUCH usa un BUS SPI SEPARATO
//     SPI e' un bus a piu' fili (clock, dati in, dati out, chip-select) condiviso
//     da piu' dispositivi. Su questa scheda (CYD) il controller touch (XPT2046)
//     e' cablato su pin diversi da quelli del display, su un secondo bus SPI
//     hardware (VSPI). Per questo la funzione touch integrata in TFT_eSPI non
//     funziona: bisogna pilotare il touch con la sua libreria sul proprio bus.
//
//  4) DEBOUNCE (antirimbalzo)
//     Un singolo tocco/click puo' generare piu' letture ravvicinate (rimbalzo
//     elettrico o dito che resta premuto). Il debounce ignora i nuovi eventi
//     per un breve tempo dopo il primo (qui 300 ms): cosi' un tocco = un'azione.
//
//  5) Mappatura colori "strana" del pannello
//     Questo specifico display rende i canali colore scambiati e bianco/nero
//     invertiti. Le macro COL_* e la funzione rgb() traducono il colore VOLUTO
//     nel valore grezzo che il pannello mostra davvero (vedi piu' sotto).
// ============================================================================

#include <Arduino.h>             // Funzioni base del framework Arduino (setup/loop, millis, map...)
#include <Wire.h>                // Libreria I2C (bus a due fili) usata dal BME280
#include <SPI.h>                 // Libreria SPI: serve per il bus dedicato del touch
#include <TFT_eSPI.h>            // Driver del display TFT (ILI9341) e degli sprite
#include <Adafruit_Sensor.h>     // Interfaccia comune dei sensori Adafruit (richiesta dal BME280)
#include <Adafruit_BME280.h>     // Libreria specifica del sensore BME280 (temp/umidita'/pressione)
#include <XPT2046_Touchscreen.h> // Driver del controller touch resistivo XPT2046
#include <WiFi.h>                // Gestione della connessione WiFi (ESP32)
#include <WiFiClientSecure.h>    // Client TLS per le richieste HTTPS
#include <HTTPClient.h>          // Client HTTP/HTTPS di alto livello (POST verso il PHP)

// --- Debug seriale: metti false per disabilitare init + tutti i Serial.print ---
#define SERIAL_DEBUG true       // Costante di compilazione: se false i rami "if (SERIAL_DEBUG)" spariscono

// --- WiFi e invio al database (ponte PHP su Aruba) ---
// I dati inviati qui sono poi consultabili come grafici sulla dashboard web
// /meteo/ (file server/index.html + server/data.php): l'ESP32 scrive via
// insert.php, la pagina legge via data.php. Vedi server/README.md.
// Le credenziali (WIFI_SSID/PASS, DB_ENDPOINT, DB_TOKEN) stanno in secrets.h,
// che NON è nel repo: copia secrets.example.h in secrets.h e metti i tuoi valori.
#include "secrets.h"
#define DB_SEND_MS 60000UL       // Ogni quanto inviare al DB (60000 ms = 1 minuto)

// --- Pin I2C del BME280 ---
#define BME_SDA 27               // Pin dati (SDA) dell'I2C verso il BME280
#define BME_SCL 22               // Pin clock (SCL) dell'I2C verso il BME280

// --- Touch XPT2046 (bus SPI dedicato del CYD, separato dal display) ---
#define T_CLK  25                // Clock SPI del touch
#define T_DIN  32                // MOSI: dati dal microcontrollore verso il touch
#define T_DOUT 39                // MISO: dati dal touch verso il microcontrollore (pin solo-input)
#define T_CS   33                // Chip Select del touch (lo "abilita" sul bus)
#define T_IRQ  36                // Pin di interrupt: segnala quando lo schermo viene toccato
// Calibrazione raw->pixel (ricavata dai tocchi sul Serial)
#define TS_MINX 230              // Valore grezzo dell'ADC touch sul bordo sinistro (x=0)
#define TS_MAXX 3855             // Valore grezzo sul bordo destro (x=240)
#define TS_MINY 237              // Valore grezzo sul bordo alto (y=0)
#define TS_MAXY 3775             // Valore grezzo sul bordo basso (y=320)

// --- Fotoresistenza (LDR) integrata sul CYD ---
#define LDR_PIN 34               // Pin analogico (ADC1, solo input) collegato all'LDR
// Calibrazione: il raw resta vicino a 0 con luce e sale al buio.
// LDR_LIGHT = raw a piena luce (-> 100%), LDR_DARK = raw coperto (-> 0%).
// Regola questi due valori leggendo "raw" dal Serial.
#define LDR_LIGHT 0              // Lettura grezza a piena luce (mappata a 100%)
#define LDR_DARK  500            // Lettura grezza al buio/coperto (mappata a 0%)

// --- Salvaschermo: spegne la retroilluminazione dopo un periodo di inattivita' ---
#define BL_PIN 21                // Pin della retroilluminazione del CYD (TFT_BL)
#define SCREEN_TIMEOUT_MS 180000UL // Inattivita' prima di spegnere (180000 ms = 3 minuti)

// --- Colori "logici" ---
// Sul tuo display i canali colore sono scambiati: queste costanti
// mappano il colore DESIDERATO sulla costante TFT_* che lo produce davvero.
// Su questo pannello anche bianco e nero risultano invertiti.
#define COL_RED    TFT_YELLOW    // Per ottenere ROSSO a schermo si passa TFT_YELLOW
#define COL_GREEN  TFT_MAGENTA   // Per ottenere VERDE puro si passa TFT_MAGENTA
#define COL_BLUE   TFT_CYAN      // Per ottenere BLU si passa TFT_CYAN
#define COL_WHITE  TFT_BLACK     // Per ottenere BIANCO si passa TFT_BLACK (nero/bianco invertiti)
#define COL_BLACK  TFT_WHITE     // Per ottenere NERO si passa TFT_WHITE
#define COL_YELLOW TFT_RED       // Per ottenere GIALLO si passa TFT_RED (ricavato dalla regola)
#define COL_GREY   0x7BEF        // Grigio medio in formato RGB565 grezzo

// Converte un colore RGB "normale" nel valore che il pannello rende davvero
// (regola: input = NOT(scambia_R_B(colore_voluto))). Cosi' si possono usare
// tinte eleganti qualsiasi senza tentativi.
uint16_t rgb(uint8_t R, uint8_t G, uint8_t B) {                 // Riceve un colore in RGB 0-255 e ritorna il valore RGB565 "corretto"
  uint16_t d = ((R & 0xF8) << 8) | ((G & 0xFC) << 3) | (B >> 3);// Comprime R,G,B in RGB565 (5+6+5 bit)
  uint16_t r5 = (d >> 11) & 0x1F, g6 = (d >> 5) & 0x3F, b5 = d & 0x1F; // Estrae i tre campi: R(5 bit), G(6 bit), B(5 bit)
  return ~(uint16_t)((b5 << 11) | (g6 << 5) | r5);              // Scambia R<->B e inverte tutti i bit (la regola del pannello)
}

// Palette stile studio di registrazione (piastra rack + VU avorio incassato)
const uint16_t COL_PLATE   = rgb(16, 16, 19);    // Sfondo scuro della "piastra" rack
const uint16_t COL_PLATE_BD= rgb(66, 68, 78);    // Bordo/luce chiara della piastra
const uint16_t COL_FACE    = rgb(234, 228, 208); // Avorio della faccia del VU-meter
const uint16_t COL_BEZEL   = rgb(20, 20, 24);    // Cornice scura attorno al quadrante
const uint16_t COL_SCALE   = rgb(35, 32, 28);    // Colore di scala, tacche e lancetta
const uint16_t COL_AMBER   = rgb(200, 150, 45);  // Arco "attenzione" (zona 55-80%)
const uint16_t COL_REDZ    = rgb(190, 45, 38);   // Arco "rosso" (zona 80-100%)
const uint16_t COL_TICK    = rgb(150, 156, 168); // Grigio delle etichette serigrafate
const uint16_t COL_READOUT = rgb(232, 228, 214); // Bianco caldo delle cifre di lettura
// LED del bargraph (peak-meter da console)
const uint16_t COL_LED_G   = rgb(80, 180, 90);   // Segmenti verdi (livello basso)
const uint16_t COL_LED_A   = rgb(215, 160, 55);  // Segmenti ambra (livello medio-alto)
const uint16_t COL_LED_R   = rgb(200, 55, 45);   // Segmenti rossi (livello alto)
const uint16_t COL_LED_OFF = rgb(46, 48, 55);    // Segmenti spenti (sfondo del bargraph)

TFT_eSPI tft = TFT_eSPI();            // Oggetto principale del display
TFT_eSprite spr = TFT_eSprite(&tft);  // Buffer fuori schermo per i VU-meter (evita lo sfarfallio)
TFT_eSprite bar = TFT_eSprite(&tft);  // Buffer fuori schermo per le strip a LED
TFT_eSprite st  = TFT_eSprite(&tft);  // Buffer fuori schermo per la barra di stato (no flicker)
Adafruit_BME280 bme;                  // Oggetto del sensore BME280

SPIClass tsSPI(VSPI);                 // Secondo bus SPI (VSPI) usato solo dal touch
XPT2046_Touchscreen ts(T_CS, T_IRQ);  // Oggetto del touch, con i suoi pin CS e IRQ

bool bmeOk = false;                   // true se il sensore e' stato trovato all'avvio
unsigned long lastUpdate = 0;         // Istante (ms) dell'ultimo aggiornamento dei dati
unsigned long lastSend = 0;           // Istante (ms) dell'ultimo invio al database
unsigned long lastTouchMs = 0;        // Istante (ms) dell'ultimo tocco gestito (antirimbalzo)
float lastTemp = NAN;                 // Ultima temperatura letta (per ridisegno immediato al tocco)
bool  tempRed  = false;               // Stato del toggle: cifre temperatura rosse si/no
byte  dbStatus = 0;                   // Esito invio DB: 0=mai, 1=OK, 2=errore
unsigned long lastDbOkMs = 0;         // Istante (ms) dell'ultimo invio andato a buon fine
unsigned long lastActivityMs = 0;     // Istante (ms) dell'ultima attivita' (tocco), per il salvaschermo
bool  screenOn = true;                // true se la retroilluminazione e' accesa

// Sprite di un gauge: tutta la larghezza dello schermo
#define GW 240                        // Larghezza dello sprite del VU-meter
#define GH 90                         // Altezza dello sprite del VU-meter
// Sprite di una strip a LED (pressione/luce)
#define BW 236                        // Larghezza dello sprite della strip
#define BH 46                         // Altezza dello sprite della strip
// Sprite della barra di stato (in fondo)
#define SW 240                        // Larghezza della barra di stato
#define SH 28                         // Altezza della barra di stato
#define SY 292                        // Posizione Y sullo schermo della barra di stato

// Gauge a lancetta (semicircolare, stile VU-meter) disegnato in uno sprite
// e posizionato a (x, y) sullo schermo.
void drawGauge(int x, int y, float value, float vmin, float vmax,
               const char *label, const char *valText, uint16_t valColor) {
  const int cx = 160;      // Coordinata X del perno della lancetta (dentro lo sprite)
  const int cy = 74;       // Coordinata Y del perno della lancetta
  const int r  = 60;       // Raggio dell'arco del quadrante
  const int lx = 46;       // Centro X della zona testo (lettura digitale) a sinistra
  const int rOut = r;      // Raggio esterno (alias di r, per leggibilita')

  float t = (value - vmin) / (vmax - vmin); // Normalizza il valore in 0..1 rispetto al fondo scala
  if (t < 0) t = 0;                          // Limita verso il basso (no valori < 0)
  if (t > 1) t = 1;                          // Limita verso l'alto (no valori > 1)

  spr.fillSprite(COL_BLACK);                 // Pulisce lo sprite riempiendolo di nero

  // Piastra rack scura, quasi a tutta larghezza (mantiene la separazione verticale)
  spr.fillRoundRect(2, 2, GW - 4, GH - 4, 6, COL_PLATE);   // Rettangolo arrotondato pieno (la piastra)
  spr.drawRoundRect(2, 2, GW - 4, GH - 4, 6, COL_PLATE_BD);// Bordo del rettangolo (cornice chiara)
  spr.drawFastHLine(7, 4, GW - 14, COL_PLATE_BD);          // Sottile linea orizzontale in alto (effetto luce)

  // VU avorio incassato (cornice scura), attorno al solo quadrante
  const int fx = 90, fy = 8, fw = 142, fh = 76;            // Posizione e dimensioni della faccia avorio
  spr.fillRoundRect(fx, fy, fw, fh, 5, COL_FACE);          // Faccia avorio piena
  spr.drawRoundRect(fx, fy, fw, fh, 5, COL_BEZEL);         // Cornice scura esterna
  spr.drawRoundRect(fx + 1, fy + 1, fw - 2, fh - 2, 4, COL_BEZEL); // Seconda cornice interna (spessore 2px)

  // Archi di zona vicino al bordo: ambra 55-80%, rosso 80-100% (come un VU reale)
  for (int a = 0; a <= 180; a++) {                         // Scorre l'arco grado per grado (0=destra, 180=sinistra)
    float tt = (180.0f - a) / 180.0f;                      // Converte l'angolo in posizione 0..1 sulla scala
    if (tt < 0.55f) continue;                              // Sotto il 55% non disegna zona colorata
    uint16_t zc = (tt < 0.80f) ? COL_AMBER : COL_REDZ;     // 55-80% ambra, oltre rosso
    float rad = a * DEG_TO_RAD, c = cosf(rad), s = sinf(rad); // Converte in radianti e calcola seno/coseno
    spr.drawLine(cx + c * rOut, cy - s * rOut, cx + c * (rOut - 4), cy - s * (rOut - 4), zc); // Traccia un segmento radiale colorato
  }

  // Linea di scala scura
  for (int a = 0; a <= 180; a++) {                         // Di nuovo lungo tutto l'arco
    float rad = a * DEG_TO_RAD, c = cosf(rad), s = sinf(rad); // Seno/coseno dell'angolo
    spr.drawPixel(cx + c * (rOut - 5), cy - s * (rOut - 5), COL_SCALE); // Un puntino scuro: disegna la linea di scala
  }

  // Tacche: maggiori ogni 18°, minori ogni 9° (scure su avorio)
  for (int i = 0; i <= 20; i++) {                          // 21 tacche (ogni 9 gradi)
    float a = (180 - i * 9) * DEG_TO_RAD, c = cosf(a), s = sinf(a); // Angolo della tacca i-esima
    int len = (i % 2 == 0) ? 9 : 5;                        // Tacca lunga se indice pari, corta se dispari
    int ro = rOut - 5;                                     // Parte le tacche appena dentro la scala
    spr.drawLine(cx + c * ro, cy - s * ro, cx + c * (ro - len), cy - s * (ro - len), COL_SCALE); // Disegna la tacca
  }

  // Lancetta scura affusolata + mozzo
  float aN = (180.0f - t * 180.0f) * DEG_TO_RAD, c = cosf(aN), s = sinf(aN); // Angolo della lancetta in base al valore t
  int tipx = cx + c * (rOut - 7), tipy = cy - s * (rOut - 7); // Punta della lancetta (vicino alla scala)
  int bw = 2;                                              // Semi-larghezza della base della lancetta
  spr.fillTriangle(cx + s * bw, cy + c * bw, cx - s * bw, cy - c * bw,
                   tipx, tipy, COL_SCALE);                 // Disegna la lancetta come triangolo (base larga, punta sottile)
  spr.fillCircle(cx, cy, 4, COL_BEZEL);                    // Mozzo: cerchietto scuro al centro
  spr.fillCircle(cx, cy, 2, COL_FACE);                     // Puntino avorio dentro il mozzo (rifinitura)

  // Lettura a sinistra (sulla piastra): etichetta serigrafata sopra, cifre sotto
  spr.setTextDatum(BC_DATUM);                              // Allinea il testo in basso-centro
  spr.setTextColor(COL_TICK, COL_PLATE);                   // Colore testo (grigio) e sfondo (piastra)
  spr.drawString(label, lx, cy - 20, 2);                  // Scrive l'etichetta (es. "TEMP.") con font 2
  spr.setTextDatum(MC_DATUM);                              // Allinea il testo al centro
  spr.setTextColor(valColor, COL_PLATE);                   // Colore delle cifre (variabile) e sfondo
  spr.drawString(valText, lx, cy - 2, 4);                 // Scrive il valore numerico con font 4 (grande)

  spr.pushSprite(x, y);                                    // Copia lo sprite sullo schermo alla posizione (x,y)
}

// Ridisegna solo il VU della temperatura (usato anche al tocco)
void drawTempGauge() {
  if (isnan(lastTemp)) return;                             // Se non c'e' ancora una lettura valida, esce
  char buf[12];                                            // Buffer di testo per la stringa del valore
  snprintf(buf, sizeof(buf), "%.1fC", lastTemp);          // Formatta la temperatura con un decimale + "C"
  drawGauge(0, 9, lastTemp, 0, 50, "TEMP.", buf, tempRed ? COL_RED : COL_READOUT); // Ridisegna scegliendo il colore in base al toggle
}

// Accende/spegne la retroilluminazione (salvaschermo)
void setBacklight(bool on) {
  digitalWrite(BL_PIN, on ? HIGH : LOW);                   // HIGH = acceso, LOW = spento (su questo CYD)
  screenOn = on;                                           // Aggiorna lo stato
}

// Gestione tocco: risveglio salvaschermo + tap sul VU temperatura -> toggle colore
void handleTouch() {
  if (millis() - lastTouchMs < 300) return;               // Antirimbalzo: ignora tocchi entro 300ms dall'ultimo
  if (!ts.touched()) return;                               // Se lo schermo non e' premuto, esce subito
  TS_Point p = ts.getPoint();                              // Legge il punto toccato (x,y grezzi + pressione z)
  lastTouchMs = millis();                                  // Memorizza l'istante del tocco (per l'antirimbalzo)
  lastActivityMs = millis();                               // Qualsiasi tocco = attivita' (resetta il salvaschermo)

  if (!screenOn) {                                         // Se lo schermo era spento...
    setBacklight(true);                                    // ...lo riaccende...
    return;                                                // ...e questo primo tocco serve SOLO a risvegliare
  }

  int sx = constrain(map(p.x, TS_MINX, TS_MAXX, 0, 240), 0, 239); // Converte x grezzo in pixel (0..239), limitato
  int sy = constrain(map(p.y, TS_MINY, TS_MAXY, 0, 320), 0, 319); // Converte y grezzo in pixel (0..319), limitato
  if (SERIAL_DEBUG)                                         // Solo in debug...
    Serial.printf("TOUCH raw(%d,%d) z=%d -> schermo(%d,%d)\n", p.x, p.y, p.z, sx, sy); // ...stampa coordinate

  // Area del VU temperatura (in cima): y sprite 9..99
  if (sy >= 9 && sy <= 99) {                               // Se il tocco cade nella fascia del VU temperatura...
    tempRed = !tempRed;                                    // ...inverte lo stato del toggle colore
    drawTempGauge();                                       // ...e ridisegna subito quel VU
  }
}

// Strip da console: piastra rack + etichetta + readout + bargraph a LED.
// Disegnata in uno sprite e posizionata a (2, y).
void drawStrip(int y, const char *label, const char *valText, float t) {
  if (t < 0) t = 0;                                        // Limita il livello a minimo 0
  if (t > 1) t = 1;                                        // Limita il livello a massimo 1

  bar.fillSprite(COL_BLACK);                               // Pulisce lo sprite della strip (nero)

  // Piastra rack coordinata con i VU
  bar.fillRoundRect(0, 0, BW, BH, 6, COL_PLATE);           // Piastra scura piena
  bar.drawRoundRect(0, 0, BW, BH, 6, COL_PLATE_BD);        // Bordo della piastra
  bar.drawFastHLine(7, 2, BW - 14, COL_PLATE_BD);          // Linea di luce superiore

  // Etichetta serigrafata a sx, readout a dx
  bar.setTextDatum(TL_DATUM);                              // Allinea il testo in alto-sinistra
  bar.setTextColor(COL_TICK, COL_PLATE);                   // Colore etichetta (grigio) su piastra
  bar.drawString(label, 10, 6, 2);                        // Scrive l'etichetta (es. "PRESSIONE")
  bar.setTextDatum(TR_DATUM);                              // Allinea il testo in alto-destra
  bar.setTextColor(COL_READOUT, COL_PLATE);                // Colore valore (bianco caldo)
  bar.drawString(valText, BW - 10, 6, 2);                 // Scrive il valore allineato a destra

  // Recesso scuro della traccia
  const int tx = 10, ty = 26, tw = BW - 20, th = 12;       // Posizione/dimensioni dell'area del bargraph
  bar.fillRoundRect(tx - 2, ty - 2, tw + 4, th + 4, 3, COL_BEZEL); // Riquadro scuro "incassato" dietro i LED

  // Bargraph a LED (verde / ambra / rosso) con segmenti spenti dietro
  const int segN = 32;                                     // Numero di segmenti LED
  float segWf = (float)tw / segN;                          // Larghezza di un segmento (in float, per precisione)
  for (int i = 0; i < segN; i++) {                         // Per ogni segmento da sinistra a destra
    float segT = (i + 0.5f) / segN;                        // Posizione 0..1 del centro del segmento
    int sx = tx + (int)(i * segWf);                        // Coordinata X di partenza del segmento
    int sw = (int)segWf - 1; if (sw < 1) sw = 1;           // Larghezza del segmento (1px di gap), minimo 1
    uint16_t col;                                          // Colore che assumera' il segmento
    if (segT > t)            col = COL_LED_OFF;            // Oltre il livello attuale: segmento spento
    else if (segT < 0.55f)   col = COL_LED_G;             // Sotto 55%: verde
    else if (segT < 0.80f)   col = COL_LED_A;             // 55-80%: ambra
    else                     col = COL_LED_R;             // Oltre 80%: rosso
    bar.fillRect(sx, ty, sw, th, col);                    // Disegna il segmento
  }

  bar.pushSprite(2, y);                                    // Copia lo sprite sullo schermo a x=2, y dato
}

// Lettura LDR mediata, per ridurre il tremolio
int readLdr() {
  long sum = 0;                                            // Accumulatore delle letture
  for (int i = 0; i < 8; i++) sum += analogRead(LDR_PIN); // Somma 8 letture dell'ADC sul pin LDR
  return sum / 8;                                          // Ritorna la media (riduce il rumore)
}

// Connette al WiFi con un timeout (non blocca per sempre se la rete manca)
void connectWifi() {
  WiFi.mode(WIFI_STA);                                    // Modalita' "station" (ci colleghiamo a un router)
  WiFi.begin(WIFI_SSID, WIFI_PASS);                       // Avvia la connessione con le credenziali
  unsigned long t0 = millis();                            // Istante di partenza, per il timeout
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 10000) // Attende fino a 10 secondi...
    delay(250);                                           // ...controllando ogni 250 ms
  if (SERIAL_DEBUG)                                        // In debug stampa l'esito
    Serial.println(WiFi.status() == WL_CONNECTED
                   ? "WiFi OK: " + WiFi.localIP().toString() // Connesso: mostra l'IP ottenuto
                   : "WiFi NON connesso");                 // Fallito: avvisa
}

// Invia una misura al database tramite POST HTTPS allo script PHP
void sendToDb(float temp, float hum, float pres, float light) {
  if (WiFi.status() != WL_CONNECTED) {                    // Se il WiFi e' caduto...
    WiFi.reconnect();                                      // ...prova a riconnettersi...
    dbStatus = 2;                                          // ...segna l'invio come errore e salta
    return;
  }
  WiFiClientSecure client;                                // Client TLS per HTTPS
  client.setInsecure();                                   // Non verifica il certificato (comodo per iniziare)
  HTTPClient https;                                       // Oggetto HTTP di alto livello
  if (https.begin(client, DB_ENDPOINT)) {                 // Apre la connessione verso l'URL del PHP
    https.addHeader("Content-Type", "application/x-www-form-urlencoded"); // Tipo dei dati: form
    // Costruisce il corpo della richiesta con i campi attesi dal PHP
    String body = "token=" + String(DB_TOKEN) +
                  "&temp="  + String(temp, 1) +
                  "&hum="   + String(hum, 1) +
                  "&pres="  + String(pres, 1) +
                  "&light=" + String(light, 0);
    int code = https.POST(body);                          // Esegue la POST e ottiene il codice HTTP
    if (code == 200) { dbStatus = 1; lastDbOkMs = millis(); } // 200 = inserito: stato OK + timestamp
    else             { dbStatus = 2; }                    // Qualsiasi altro codice: errore
    if (SERIAL_DEBUG)                                       // In debug stampa codice e risposta del server
      Serial.printf("DB POST -> %d: %s\n", code, https.getString().c_str());
    https.end();                                          // Chiude la connessione
  } else {
    dbStatus = 2;                                          // begin() fallita: errore
    if (SERIAL_DEBUG) Serial.println("DB POST: begin() fallita"); // L'URL non e' valido/raggiungibile
  }
}

// Riempie un "settore di torta" (da ore 12, in senso orario) nello sprite di stato
void fillPie(int cx, int cy, int r, float frac, uint16_t col) {
  int aMax = (int)(frac * 360);                            // Quanti gradi riempire
  for (int a = 0; a < aMax; a++) {                         // Per ogni grado da 0 a aMax...
    float rad = (a - 90) * DEG_TO_RAD;                     // -90 = ore 12; crescendo va in senso orario
    st.drawLine(cx, cy, cx + cosf(rad) * r, cy + sinf(rad) * r, col); // Raggio dal centro al bordo (riempie)
  }
}

// Barra di stato in basso: icona WiFi (sx) e icona DB con avanzamento (dx).
// Disegnata in uno sprite e poi spinta sullo schermo -> niente sfarfallio.
void drawStatusBar() {
  st.fillSprite(COL_BLACK);                                // Pulisce lo sprite (coordinate locali 0..SH)

  // --- WiFi: barre di segnale (verdi se connesso, rosse se no) ---
  bool w = (WiFi.status() == WL_CONNECTED);                // Stato attuale del WiFi
  int level = 0;                                           // Tacche accese (0..4) in base all'RSSI
  if (w) {                                                 // Solo se connesso calcola la potenza
    long rssi = WiFi.RSSI();                               // Potenza del segnale in dBm (negativa)
    level = (rssi >= -55) ? 4 : (rssi >= -65) ? 3 : (rssi >= -75) ? 2 : 1; // Soglie -> 1..4 tacche
  }
  const int bx = 10, by = 23;                              // Origine (in basso a sinistra) delle barre
  for (int i = 0; i < 4; i++) {                            // Quattro barre crescenti
    int h = 5 + i * 3;                                     // Altezza della barra i-esima
    int x = bx + i * 6;                                    // X della barra i-esima
    if (!w)                                                // Non connesso: barra piena rossa
      st.fillRect(x, by - h, 4, h, COL_RED);
    else if (i < level)                                    // Connesso e nel livello: barra piena verde
      st.fillRect(x, by - h, 4, h, COL_GREEN);
    else                                                   // Connesso ma oltre il livello: solo contorno
      st.drawRect(x, by - h, 4, h, COL_TICK);
  }

  // --- DB: icona a cilindro (database) + cerchio che si riempie nel minuto ---
  uint16_t dbc = (dbStatus == 2) ? COL_RED                 // Rosso se l'ultimo invio e' in errore
               : (dbStatus == 1) ? COL_GREEN               // Verde se l'ultimo invio e' OK
               : COL_TICK;                                 // Grigio se non e' ancora stato fatto
  const int dx = 190;                                      // Centro X del cilindro
  st.drawEllipse(dx, 5, 7, 3, dbc);                        // Disco superiore (la "tappa" del cilindro)
  st.drawFastVLine(dx - 7, 5, 13, dbc);                    // Fianco sinistro
  st.drawFastVLine(dx + 7, 5, 13, dbc);                    // Fianco destro
  st.drawEllipse(dx, 11, 7, 3, dbc);                       // Disco intermedio (effetto "dischi")
  st.drawEllipse(dx, 18, 7, 3, dbc);                       // Disco inferiore (base)

  // Cerchio di avanzamento verso il prossimo invio (0..1 nel minuto)
  float frac = (float)(millis() - lastSend) / DB_SEND_MS;  // Frazione di minuto trascorsa
  if (frac < 0) frac = 0;                                  // Limita sotto
  if (frac > 1) frac = 1;                                  // Limita sopra
  const int ccx = 222, ccy = 12, cr = 8;                   // Centro e raggio del cerchio (coord. locali)
  fillPie(ccx, ccy, cr, frac, dbc);                        // Riempie la torta in base all'avanzamento
  st.drawCircle(ccx, ccy, cr, COL_TICK);                   // Contorno del cerchio

  st.pushSprite(0, SY);                                    // Copia la barra sullo schermo in fondo
}

void setup() {                                             // Eseguita una volta all'avvio
  if (SERIAL_DEBUG) Serial.begin(115200);                  // Avvia la seriale solo se il debug e' attivo

  tft.init();                                              // Inizializza il display
  tft.setRotation(0);                                      // Orientamento verticale (240x320)
  tft.fillScreen(COL_BLACK);                               // Riempie lo schermo di nero

  analogReadResolution(12);                                // Imposta l'ADC a 12 bit (valori 0..4095)

  pinMode(BL_PIN, OUTPUT);                                  // Pin retroilluminazione come uscita
  digitalWrite(BL_PIN, HIGH);                               // Schermo acceso all'avvio
  lastActivityMs = millis();                               // Avvia il timer del salvaschermo

  spr.setColorDepth(16);                                   // Sprite VU a 16 bit per pixel (colore RGB565)
  spr.createSprite(GW, GH);                                // Alloca in RAM lo sprite dei VU-meter
  bar.setColorDepth(16);                                   // Sprite strip a 16 bit per pixel
  bar.createSprite(BW, BH);                                // Alloca in RAM lo sprite delle strip
  st.setColorDepth(16);                                    // Sprite barra di stato a 16 bit per pixel
  st.createSprite(SW, SH);                                 // Alloca in RAM lo sprite della barra di stato

  // Touch su bus SPI dedicato (VSPI): CLK, MISO, MOSI, CS
  tsSPI.begin(T_CLK, T_DOUT, T_DIN, T_CS);                 // Avvia il bus SPI del touch sui pin indicati
  ts.begin(tsSPI);                                         // Inizializza il driver touch su quel bus
  ts.setRotation(0);                                       // Orientamento del touch coerente col display

  connectWifi();                                           // Si connette al WiFi (con timeout) per inviare al DB

  // --- Inizializzazione BME280 ---
  Wire.begin(BME_SDA, BME_SCL);                            // Avvia l'I2C sui pin del BME280
  bmeOk = bme.begin(0x76) || bme.begin(0x77);              // Prova l'indirizzo 0x76, poi 0x77; salva l'esito
  if (SERIAL_DEBUG) Serial.println(bmeOk ? "BME280 trovato!" : "BME280 NON trovato"); // Stampa diagnostica

  if (!bmeOk) {                                            // Se il sensore non risponde...
    tft.setTextDatum(MC_DATUM);                            // Allinea il testo al centro
    tft.setTextColor(COL_RED, COL_BLACK);                  // Testo rosso su nero
    tft.drawString("Sensore non trovato", tft.width() / 2, 160, 2); // Mostra un messaggio d'errore a schermo
  }
}

void loop() {                                              // Eseguita in continuazione dopo setup
  if (!bmeOk) return;                                      // Senza sensore non fa nulla

  handleTouch(); // reattivo ad ogni giro, indipendente dall'aggiornamento dati  // Controlla il touch ogni ciclo

  // Salvaschermo: dopo l'inattivita' spegne la retroilluminazione (il touch la riaccende)
  if (screenOn && millis() - lastActivityMs >= SCREEN_TIMEOUT_MS)
    setBacklight(false);

  if (millis() - lastUpdate >= 1000) {                     // Ogni 1000 ms (1 secondo)...
    lastUpdate = millis();                                 // Aggiorna il timer dell'ultimo refresh

    float temp = bme.readTemperature();        // C        // Legge la temperatura in gradi Celsius
    float hum  = bme.readHumidity();           // %        // Legge l'umidita' relativa in percentuale
    float pres = bme.readPressure() / 100.0f;  // hPa       // Legge la pressione in Pa e converte in hPa
    lastTemp = temp;                                       // Memorizza la temperatura per il ridisegno al tocco

    int   raw   = readLdr();                               // Legge (mediata) la fotoresistenza
    // piu' luce = raw piu' basso; riscalato sull'intervallo utile e limitato 0..100
    float light = (LDR_DARK - raw) * 100.0f / (LDR_DARK - LDR_LIGHT); // Converte il raw LDR in percentuale di luce
    if (light < 0) light = 0;                              // Limita a minimo 0%
    if (light > 100) light = 100;                          // Limita a massimo 100%

    if (SERIAL_DEBUG)                                       // Solo in debug...
      Serial.printf("T=%.1fC  H=%.1f%%  P=%.1fhPa  Luce=%.0f%% (raw=%d)\n",
                    temp, hum, pres, light, raw);          // ...stampa tutti i valori sulla seriale

    char buf[12];                                          // Buffer per formattare i testi numerici

    // VU-meter impilati: Temp in cima, poi Umidita' (gap verticali compatti ~5px)
    snprintf(buf, sizeof(buf), "%.1fC", temp);             // Prepara la stringa della temperatura
    drawGauge(0, 9, temp, 0, 50, "TEMP.", buf, tempRed ? COL_RED : COL_READOUT); // Disegna il VU temperatura (colore da toggle)
    snprintf(buf, sizeof(buf), "%.0f%%", hum);             // Prepara la stringa dell'umidita'
    drawGauge(0, 100, hum, 0, 100, "UMID.", buf, COL_READOUT); // Disegna il VU umidita'

    // Strip a LED da console: Pressione poi Luce
    snprintf(buf, sizeof(buf), "%.0f hPa", pres);          // Prepara la stringa della pressione
    drawStrip(193, "PRESSIONE", buf, (pres - 950) / 100.0f); // Disegna la strip pressione (scala 950..1050 hPa)
    snprintf(buf, sizeof(buf), "%.0f%%", light);           // Prepara la stringa della luce
    drawStrip(244, "LUCE", buf, light / 100.0f);           // Disegna la strip luce (scala 0..100%)

    // Invio periodico al database (ogni DB_SEND_MS, qui 1 minuto)
    if (millis() - lastSend >= DB_SEND_MS) {               // Se e' passato l'intervallo di invio...
      lastSend = millis();                                 // ...aggiorna il timer...
      sendToDb(temp, hum, pres, light);                    // ...e manda l'ultima misura al DB
    }

    drawStatusBar();                                       // Aggiorna in basso lo stato WiFi e DB
  }
}
