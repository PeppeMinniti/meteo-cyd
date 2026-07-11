# CYD - BME280

Stazione ambientale su **CYD (ESP32-2432S028)** con display ILI9341 240x320 (portrait)
e sensore **BME280**. Mostra temperatura e umidità come VU-meter a lancetta e
pressione + luce ambientale come slider.

## Build
- PlatformIO, env `esp32dev`. Librerie: `TFT_eSPI`, `Adafruit BME280`, `Adafruit Unified Sensor`, `XPT2046_Touchscreen`.
- La configurazione del display è interamente nei `build_flags` di `platformio.ini`
  (non c'è `User_Setup.h`).
- **Credenziali**: WiFi e token DB stanno in `src/secrets.h` (ignorato da git).
  Prima di compilare: `cp src/secrets.example.h src/secrets.h` e inserire i valori.

## Hardware
- **BME280** su I2C: `SDA = GPIO27`, `SCL = GPIO22` → `Wire.begin(27, 22)`,
  indirizzo 0x76 (fallback 0x77).
- **LDR** integrata: `GPIO34` (ADC, input-only). Più luce ⇒ raw più basso.
  Calibrazione con `LDR_LIGHT` / `LDR_DARK` (vedi `raw` su Serial @115200).
- **Touch XPT2046**: bus SPI **separato** dal display (`tft.getTouch()` NON funziona).
  Pin: CLK=25, MOSI=32, MISO=39, CS=33, IRQ=36 su `SPIClass(VSPI)` con la lib
  `XPT2046_Touchscreen`. Calibrazione (rot. 0): X 230–3855, Y 237–3775.

## Colori del display (IMPORTANTE)
Questo pannello rende i colori **scambiati e con bianco/nero invertiti**.
Usare SEMPRE le macro `COL_*` in `src/main.cpp`, mai le costanti `TFT_*` grezze.

Regola per ricavare un colore qualsiasi:
`valore_da_passare = NOT( scambia_R_e_B( colore_voluto ) )` in RGB565.

| Colore voluto | Costante da usare |
|---------------|-------------------|
| Rosso  | `TFT_YELLOW` |
| Verde  | `TFT_MAGENTA` (verde puro; `TFT_PINK`/`TFT_PURPLE` = verde sporco) |
| Blu    | `TFT_CYAN`   |
| Giallo | `TFT_RED`    |
| Bianco | `TFT_BLACK`  |
| Nero   | `TFT_WHITE`  |

In `src/main.cpp` c'è anche l'helper `rgb(R,G,B)` che applica la regola a un colore
RGB qualsiasi (così non servono tentativi).

NON usare `tft.invertDisplay()`: romperebbe questa taratura.

## Rendering
- I VU-meter e le strip a LED sono disegnati in `TFT_eSprite` riusati e poi
  `pushSprite()` → niente flicker.
- Touch in polling ad ogni `loop()` (debounce 300ms); tap sul VU temperatura
  inverte il colore della lettura (bianco↔rosso).
- Barra di stato in basso (sprite `st`): icone WiFi (barre segnale) e DB
  (cilindro + cerchio che si riempie nel minuto).
- Salvaschermo: retroilluminazione (GPIO21/TFT_BL) spenta dopo 3 min di
  inattività (`SCREEN_TIMEOUT_MS`); un tocco riaccende.
- Debug seriale governato dal flag `#define SERIAL_DEBUG` in cima al file.

## Invio dati al database
- L'ESP32 si connette al WiFi e invia le misure ogni minuto via **POST HTTPS** a
  uno script PHP (`server/insert.php`) sullo spazio web Aruba, che scrive nel
  MySQL (tabella `letture_meteo`). Il MySQL condiviso Aruba non accetta
  connessioni remote: per questo si passa dal ponte PHP. Le credenziali DB e il
  token stanno in `server/secrets.php` (vedi `server/secrets.example.php`).
- Config: `WIFI_SSID/PASS`, `DB_ENDPOINT`, `DB_TOKEN` in `src/secrets.h`
  (vedi `src/secrets.example.h`); `DB_SEND_MS` in `main.cpp`. L'endpoint deve
  essere l'URL **finale** (con `www.`, niente 301).
- Setup server: vedi `server/README.md`.

## Dashboard web (`/meteo/`)
Pagina pubblica su `www.peppeminniti.it/meteo/` che mostra valori attuali e
grafici di temperatura/umidità/pressione/luce dal DB. Due file in `server/`,
da caricare nella stessa cartella di `insert.php`:
- `data.php`: API di **sola lettura**. Torna JSON via `?range=24h|7g|30g|anno|tutto`.
  Lettura **pubblica** (no token); aggrega per media (per ora su 7g/30g, per
  giorno su anno/tutto) per restare leggera. Stesse credenziali DB di `insert.php`.
- `index.html`: 4 grafici **Chart.js** (CDN) + card valori attuali + selettore
  intervallo + auto-refresh ogni minuto. **Click su un grafico** = fullscreen,
  altro click/`Esc` = ritorno (salva/ripristina lo scroll).
- Sotto ogni grafico riga **min · media · max** del periodo. Hover su `min`/`max`
  apre il tooltip sul punto relativo; hover su `media` mostra una linea
  tratteggiata della media (plugin `avgLine`, visibile solo in hover).
- Stile grafici (preferenze utente): linea sottile (`borderWidth 0.5`) e curve
  morbide (`tension 0.4` + `monotone`); tooltip semi-trasparenti
  (`rgba(0,0,0,0.35)`); punto in hover bianco con anello scuro (non ingrandito).
- Gotcha CSS: i `.chart-box` hanno `min-width:0`, altrimenti la canvas residua
  dal fullscreen gonfia la colonna della grid e i grafici escono dallo schermo.
  L'area `.cv` è 216px (non 240) per far stare la riga statistiche senza alzare
  la pagina.
- Campo **Altitudine (m)** a destra del selettore periodo (salvato in
  `localStorage`): traccia sul grafico pressione una linea tenue sempre
  visibile alla pressione "normale" ISA per quella quota (`normalPressure()`).
  Serve perché la pressione **misurata** cala con l'altitudine (~1 hPa ogni 8 m):
  senza un riferimento tarato sul luogo del dispositivo, un valore "basso" in
  collina/montagna sembrerebbe erroneamente bassa pressione. Confrontando la
  linea misurata con questa **linea ideale della quota** si legge a colpo
  d'occhio se si è in condizione di **alta o bassa pressione reale** per quel posto.
- **Buchi nei dati** (sensore/WiFi giù): `withGapFill()` in `index.html` nota
  che tra due letture consecutive è passato più del previsto e inserisce un
  punto "riportato" (stesso valore dell'ultima lettura reale, `filled:true`)
  fino a poco prima del prossimo dato vero — niente più interpolazione lineare
  silenziosa tra il prima e il dopo. Quei tratti sono **tratteggiati e
  smorzati** (Chart.js `segment` styling, non solo colore) e il tooltip
  segnala "dato non disponibile · ultimo valore noto"; la riga di stato in
  basso lo riepiloga a colpo d'occhio quando capita. **Le statistiche
  min/media/max escludono sempre i punti `filled`** (altrimenti un valore
  ripetuto per un'ora/giorno intero falserebbe la media).
- Tutti i file `server/` hanno commenti didattici; il progetto è didattico a
  tutti gli effetti (anche `main.cpp` è commentato riga per riga).
- `server/favicon.svg`: icona della scheda browser (sole/nuvola/pioggia/fulmine).

## Case 3D (cartella `cad/`)
- File in `cad/`: `Meteo.3mf` (progetto Bambu Studio già affettato, **file principale**),
  `Fronte.stl` (cornice inclinata reggi-display) e `Retro.stl` (guscio posteriore).
  Display e sensore BME280 si fissano con **viti M2 × 5 mm** (non a incastro).
- Profilo di stampa nel 3mf: **Bambu Lab A1 mini**, PETG (PLA ok), ugello 0.4,
  layer 0.2, infill 15%, piatto PEI Testurizzato, supporti ad albero auto, brim auto.
- Modello pubblicato su MakerWorld (licenza CC BY 4.0):
  https://makerworld.com/en/models/3038452-weather-station-case-cyd-esp32-2432s028-bme280

## Roadmap / Espansioni future
- Pilotare **attuatori esterni** (ventilatore, luce) via relè/MOSFET dalla stessa
  centralina touch.
- **Automazioni a soglia**: accensione/spegnimento automatico in base a
  **temperatura** e **luce ambientale**; comando **manuale dal touchscreen** in
  aggiunta all'automatico.
