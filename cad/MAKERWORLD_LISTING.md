# MakerWorld – testi pronti per il listing / ready-to-paste listing text

> Copia-incolla nei campi del wizard di upload MakerWorld.
> Licenza scelta: **CC BY (Attribution)**. Stampante di riferimento: **Bambu Lab A1 mini**.

---

## Titolo / Title (max ~60 caratteri)

**Weather Station Case for CYD (ESP32-2432S028) + BME280**

Alternativa più corta: `CYD Weather Station Case — BME280`

---

## Descrizione / Description  (campo unico, bilingue)

### 🇮🇹 Italiano

Case da scrivania per trasformare una **CYD (Cheap Yellow Display, ESP32-2432S028** con schermo ILI9341 240×320) in una piccola **stazione ambientale**. Il display viene tenuto inclinato dal frontale; sul retro trova posto la scheda. Abbinato a un sensore **BME280** misura **temperatura, umidità e pressione**; una **fotoresistenza (LDR)** già presente sulla CYD legge la luce ambientale.

Il firmware mostra temperatura e umidità come **VU-meter a lancetta** e pressione + luce come **slider a LED**, con barra di stato (WiFi + stato database). Ogni minuto le misure vengono inviate via WiFi a un piccolo backend che le salva su database e le pubblica su una **dashboard web** con grafici storici.

Due pezzi da stampare: **Fronte** (cornice inclinata reggi-display) e **Retro** (guscio posteriore). Display e sensore si fissano con **viti M2 da 5 mm**.

🔧 **In arrivo:** essendo touch, la centralina sarà espansa per **pilotare attuatori esterni** (es. ventilatore o luce) e **accenderli/spegnerli in automatico** in base a temperatura e luce ambientale — oltre al comando manuale a schermo.

### 🇬🇧 English

Desktop case that turns a **CYD (Cheap Yellow Display, ESP32-2432S028** board with a 240×320 ILI9341 screen) into a small **environmental / weather station**. The front bezel holds the display at a viewing angle; the board sits in the rear shell. Paired with a **BME280** sensor it reads **temperature, humidity and pressure**, and the CYD's built-in **LDR** reads ambient light.

The firmware renders temperature and humidity as **needle VU-meters** and pressure + light as **LED sliders**, with a status bar (WiFi + database status). Every minute the readings are pushed over WiFi to a small backend that stores them and serves a **web dashboard** with historical charts.

Two printed parts: **Fronte** (angled front bezel that holds the display) and **Retro** (rear shell). Display and sensor are fixed with **M2 × 5 mm screws**.

🔧 **Coming soon:** being a touchscreen, the station will be expanded to **drive external actuators** (e.g. a fan or a light) and **switch them on/off automatically** based on temperature and ambient light — in addition to on-screen manual control.

---

## Componenti / Bill of materials (non stampati / not printed)

- **CYD board** — ESP32-2432S028 (Cheap Yellow Display), ILI9341 240×320, touch XPT2046
- **BME280** sensor (I²C, module) — temp / humidity / pressure
- **Viti M2 × 5 mm / M2 × 5 mm screws** — per fissare display e sensore / to fasten the display and the sensor
- Jumper wires (I²C: SDA→GPIO27, SCL→GPIO22; VCC 3V3, GND)
- USB-C or Micro-USB cable for power (a seconda della revisione CYD / depending on CYD revision)

> LDR (light sensor) è già a bordo della CYD (GPIO34) — nessun componente extra. / The LDR is already on the CYD board (GPIO34) — no extra part.

---

## Impostazioni di stampa / Print settings (già nel .3mf / already in the .3mf)

| | |
|---|---|
| Printer / Stampante | Bambu Lab A1 mini |
| Material / Materiale | **PETG** (va bene anche PLA / PLA is fine too) |
| Nozzle / Ugello | 0.4 mm |
| Layer height / Altezza layer | 0.2 mm |
| Infill | 15% |
| Plate / Piatto | Textured PEI |
| Supports / Supporti | Tree (auto) |
| Brim | Auto |
| Parts / Pezzi | Fronte ×1, Retro ×1 |

Consiglio / Tip: stampa il **Fronte** con la faccia dello schermo verso il basso o sul retro per una cornice pulita; i supporti auto ad albero bastano per gli sbalzi. / Print **Fronte** with the screen face down/back for a clean bezel; auto tree supports handle the overhangs.

---

## Montaggio / Assembly

1. **IT** — Fissa il display CYD al **Fronte** con **viti M2 × 5 mm**, schermo verso l'apertura. **EN** — Fasten the CYD display to the **Fronte** bezel with **M2 × 5 mm screws**, screen facing the opening.
2. **IT** — Collega il **BME280** all'I²C (SDA→GPIO27, SCL→GPIO22, 3V3, GND) e avvitalo con **viti M2 × 5 mm**. **EN** — Wire the **BME280** to I²C (SDA→GPIO27, SCL→GPIO22, 3V3, GND) and fasten it with **M2 × 5 mm screws**.
3. **IT** — Sistema la scheda e i cavi nel **Retro**, fai uscire il cavo USB di alimentazione. **EN** — Place the board and cables into the **Retro** shell, route the USB power cable out.
4. **IT** — Accoppia Fronte e Retro. **EN** — Join Fronte and Retro.

---

## Software / Firmware

- Firmware **ESP32 (PlatformIO / Arduino)** con librerie TFT_eSPI, Adafruit BME280, XPT2046_Touchscreen.
- Dashboard web di esempio con grafici storici (Chart.js).
- **IT:** il firmware non è incluso nel modello 3D — link al codice qui sotto. **EN:** the firmware is not part of the 3D model — source link below.

**Link (compila / fill in):**
- GitHub (firmware): https://github.com/PeppeMinniti/meteo-cyd
- Dashboard demo: `<es. www.peppeminniti.it/meteo/>`
- Social: `<Instagram / YouTube / sito>`

---

## Tag consigliati / Suggested tags

`esp32` · `cyd` · `esp32-2432s028` · `bme280` · `weather-station` · `iot` · `case` · `enclosure` · `display` · `arduino` · `home` · `sensor`

## Categoria / Category

Hobby & DIY → Electronics  (oppure / or: Gadgets → Desk)

---

## Ordine immagini galleria / Gallery image order

1. **`finito_in_funzione_new.jpeg`** — *COVER*. Foto reale del case montato con display acceso (già quadrata 3024×3024). IT: *"Stazione ambientale CYD: display acceso su supporto da scrivania."* / EN: *"CYD environmental station: screen on, desktop stand."*
2. **`display_zoom.jpeg`** — primo piano dell'interfaccia (VU-meter + slider). IT: *"Interfaccia: temperatura e umidità a lancetta, pressione e luce a barra."* / EN: *"UI: needle gauges for temp/humidity, bar sliders for pressure/light."*
3. **`centralina_meteo_web.png`** — dashboard web con grafici storici. IT: *"Dashboard web: dati live e grafici storici."* / EN: *"Web dashboard: live data and historical charts."*
4. **`interno_assemblato.jpeg`** — CYD cablata dentro il case (viti M2 + BME280). IT: *"Montaggio: scheda e sensore fissati con viti M2 × 5 mm."* / EN: *"Assembly: board and sensor fixed with M2 × 5 mm screws."*
5. **`fronte.png`** — render del frontale/cornice reggi-display. IT: *"Fronte: cornice inclinata reggi-display."* / EN: *"Fronte: angled display bezel."*
6. **`fronte_interno.png`** — render dell'interno con le colonnine dei fori M2. IT: *"Interno: colonnine e fori per le viti M2."* / EN: *"Interior: standoffs and M2 screw holes."*
7. **`retro.png`** — render del guscio posteriore. IT: *"Retro: guscio posteriore."* / EN: *"Retro: rear shell."*

> ⚠️ La **cover** deve essere una foto reale del pezzo stampato (MakerWorld premia le foto reali). `centralina_meteo_web.png` è uno screenshot software: ottima come **immagine secondaria**, mai come cover. / The **cover** must be a real photo of the printed part; the dashboard screenshot is a great **secondary** image, never the cover.
> ℹ️ Verifica che i nomi dei render (Fronte / Retro) corrispondano ai pezzi giusti prima di pubblicare. / Double-check the Fronte/Retro render names match the right parts before publishing.

---

## Roadmap / Prossime espansioni · Future expansions

**IT** — Il progetto è in evoluzione. Prossimi step:
- Pilotaggio di **attuatori esterni** (ventilatore, luce) tramite relè/MOSFET dalla stessa centralina.
- **Automazioni a soglia**: accensione/spegnimento automatico in base a **temperatura** e **luce ambientale** (es. ventola sopra una certa temperatura, luce quando è buio).
- Comando **manuale dal touchscreen** in aggiunta all'automatico.

**EN** — Work in progress. Next steps:
- Driving **external actuators** (fan, light) via relay/MOSFET from the same station.
- **Threshold automations**: auto on/off based on **temperature** and **ambient light** (e.g. fan above a set temperature, light when it gets dark).
- **Manual touchscreen** control alongside the automatic mode.

---

## Ringraziamenti / Credits

**IT** — Ideazione, progettazione, scelte tecniche, stampa e prove sul campo sono
di **Giuseppe Minniti**. Parte dello sviluppo (firmware, dashboard, documentazione)
è stata portata avanti in **collaborazione con un assistente AI (Claude, di
Anthropic)**, usato come compagno di lavoro. Mi piace pensarla come una
collaborazione in cui si cresce a vicenda: la direzione, il senso critico e le
decisioni restano umane, l'AI le affianca e le accelera.

**EN** — Concept, design, engineering choices, printing and field testing are by
**Giuseppe Minniti**. Part of the development (firmware, dashboard, docs) was
carried out in **collaboration with an AI assistant (Claude, by Anthropic)**, used
as a working companion. I like to see it as a partnership where both sides grow:
direction, judgement and decisions stay human — the AI supports and speeds them up.

Firmware open source: https://github.com/PeppeMinniti/meteo-cyd

---

## Nota licenza / License note

Pubblicato come **CC BY 4.0**: chiunque può stampare, modificare e rimixare (anche a fini commerciali) **citando l'autore**. / Published under **CC BY 4.0**: anyone may print, modify and remix (including commercially) **with attribution**.
