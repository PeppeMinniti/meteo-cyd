# Lato server (Aruba) - centralina meteo

Questi file fanno da ponte tra l'ESP32 e il tuo database MySQL su Aruba.
Il MySQL condiviso di Aruba **non accetta connessioni remote**: l'ESP32 non si
collega direttamente al DB, ma chiama uno script PHP che gira sullo spazio web
Aruba (per cui il DB è "locale").

> 🔑 **Credenziali**: nome DB, utente, password e token stanno in `secrets.php`
> (fuori dal repo). Copia `secrets.example.php` in `secrets.php`, compila i tuoi
> valori e caricalo accanto a `insert.php` / `data.php`. Vedi i passi sotto.

## Passi

1. **Crea la tabella**
   - Entra in **phpMyAdmin** dal pannello Aruba, seleziona il tuo database.
   - Scheda **SQL** → incolla il contenuto di [`schema.sql`](schema.sql) → Esegui.

2. **Configura le credenziali (`secrets.php`)**
   - Copia [`secrets.example.php`](secrets.example.php) in `secrets.php` e compila:
     - `$DB_NAME`, `$DB_USER`, `$DB_PASS` (li trovi nel pannello Aruba; l'utente è
       di solito il nome del DB).
     - `$DB_HOST`: prova `localhost`; se non va, usa l'host MySQL del pannello.
     - `$SECRET`: una stringa lunga e casuale (es. 32 caratteri). **Deve essere
       identica** a `DB_TOKEN` nel firmware ESP32.

3. **Carica i file**
   - Via FTP o File Manager Aruba metti `insert.php` **e `secrets.php`** nella
     stessa cartella, es. `/meteo/` → l'URL sarà `https://TUOSITO/meteo/insert.php`.

4. **Imposta il firmware**
   - In `src/secrets.h` compila `WIFI_SSID`, `WIFI_PASS`, `DB_ENDPOINT`
     (l'URL del PHP) e `DB_TOKEN` (= `$SECRET`).

## Test rapido
- Apri l'URL del PHP nel browser: deve rispondere `{"ok":false,"err":"method"}`
  (giusto: rifiuta le GET). Significa che il file è raggiungibile.
- Con l'ESP32 acceso e `SERIAL_DEBUG true`, sul Serial vedrai il codice HTTP e la
  risposta JSON di ogni invio (`{"ok":true,"id":...}` se è andato a buon fine).

## Dashboard web (`/meteo/`)

Per vedere dati e grafici su `https://www.peppeminniti.it/meteo/`:

1. **Carica due file** nella stessa cartella di `insert.php` (es. `/meteo/`):
   - [`data.php`](data.php) — endpoint di **sola lettura** che torna le letture in
     JSON. Usa lo stesso `secrets.php` di `insert.php` (nessuna credenziale nel file).
   - [`index.html`](index.html) — la pagina con i 4 grafici (temperatura, umidità,
     pressione, luce) + i valori attuali. Usa Chart.js da CDN (serve internet).
2. Apri `https://www.peppeminniti.it/meteo/` → la pagina chiama `data.php` e disegna
   i grafici. Selettore intervallo: **24h / 7g / 30g / anno / tutto**; auto-refresh
   ogni minuto.

Note:
- La **lettura è pubblica** (niente token): i dati meteo sono innocui. Le credenziali
  DB restano in `secrets.php` lato server, mai esposte al browser.
- Test rapido: apri `data.php?range=24h` nel browser → deve rispondere un JSON
  `{"ok":true,...,"points":[...]}`.
- Negli intervalli lunghi i dati sono **aggregati per media** (per ora su 7g/30g,
  per giorno su anno/tutto) così il grafico resta leggero.

## Nota sicurezza
- Il token è una protezione minima; per un uso serio metti il PHP **solo in
  HTTPS** (Aruba di solito ha il certificato) ed eventualmente limita per IP.
- Il firmware usa `client.setInsecure()` (non verifica il certificato TLS): è
  comodo per iniziare; per indurire la sicurezza si può inserire il fingerprint
  o la CA del certificato.
