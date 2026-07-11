<?php
// ============================================================================
//  data.php - Endpoint di sola LETTURA per la dashboard /meteo/.
//  File commentato a scopo didattico.
//
//  A COSA SERVE
//  ---------------------------------------------------------------------------
//  La pagina index.html non puo' parlare direttamente col database MySQL: il
//  JavaScript gira nel browser dell'utente e il MySQL di Aruba non accetta
//  connessioni dall'esterno. Serve quindi un piccolo programma lato server
//  (questo PHP) che: si collega al DB "da dentro" Aruba, legge le misure e le
//  restituisce in un formato che il JavaScript sa leggere: il JSON.
//
//  Questo schema (browser -> chiama un URL -> riceve JSON) e' una "API web":
//  insert.php e' l'API di SCRITTURA (la usa l'ESP32), data.php e' l'API di
//  LETTURA (la usa la pagina). Le due cose sono separate apposta.
//
//  TEORIA - concetti chiave
//  ---------------------------------------------------------------------------
//  1) JSON: un testo strutturato (oggetti {chiave:valore} e liste [...]) che
//     sia PHP sia JavaScript sanno produrre/leggere. E' il "linguaggio comune"
//     tra server e browser. Qui lo produciamo con json_encode().
//  2) Parametro in query string: il pezzo dopo "?" nell'URL
//     (es. data.php?range=24h). PHP lo legge in $_GET['range']. Lo usiamo per
//     scegliere quanti dati e con che dettaglio restituire.
//  3) AGGREGAZIONE: con una misura al minuto, 30 giorni fanno ~43.000 righe:
//     troppe da scaricare e da disegnare. Per gli intervalli lunghi chiediamo a
//     MySQL la MEDIA per "fascia temporale" (per ora o per giorno) con
//     GROUP BY: il grafico resta leggero senza perdere l'andamento.
//  4) WHITELIST anti-injection: il valore di $range NON viene mai infilato
//     nella query SQL. Lo confrontiamo con un elenco fisso (switch) e usiamo
//     solo frammenti SQL scritti da noi. Cosi' un parametro malevolo non puo'
//     modificare la query.
//
//  Uso:  data.php?range=24h | 7g | 30g | anno | tutto
// ============================================================================

// --- CONFIGURAZIONE: stesse credenziali di insert.php, in secrets.php -------
// secrets.php (NON nel repo) definisce $DB_HOST, $DB_NAME, $DB_USER, $DB_PASS.
require __DIR__ . '/secrets.php';
// ---------------------------------------------------------------------------

// Diciamo al browser che la risposta e' JSON (cosi' fetch() la interpreta bene).
header('Content-Type: application/json; charset=utf-8');
// CORS: consente di leggere questi dati anche da una pagina ospitata altrove
// (utile per test in locale). I dati sono pubblici, quindi e' sicuro.
header('Access-Control-Allow-Origin: *');

// Intervallo richiesto -> [finestra temporale, espressione di "bucket" per il
// GROUP BY]. Se il bucket e' null i dati sono grezzi (nessuna aggregazione).
// Lo switch funge anche da whitelist: qualsiasi range non previsto -> errore.
$range = $_GET['range'] ?? '24h';  // se manca il parametro, default a 24 ore
switch ($range) {
  case '24h':   $where = 'ts >= NOW() - INTERVAL 1 DAY';    $bucket = null;                                   break; // dati grezzi al minuto
  case '7g':    $where = 'ts >= NOW() - INTERVAL 7 DAY';    $bucket = "DATE_FORMAT(ts, '%Y-%m-%d %H:00:00')"; break; // media per ORA
  case '30g':   $where = 'ts >= NOW() - INTERVAL 30 DAY';   $bucket = "DATE_FORMAT(ts, '%Y-%m-%d %H:00:00')"; break; // media per ORA
  case 'anno':  $where = 'ts >= NOW() - INTERVAL 1 YEAR';   $bucket = "DATE(ts)";                             break; // media per GIORNO
  case 'tutto': $where = '1';                               $bucket = "DATE(ts)";                             break; // tutto lo storico, media per GIORNO
  default:
    http_response_code(400);                                // 400 = richiesta non valida
    echo json_encode(['ok' => false, 'err' => 'range']);
    exit;
}

// Connessione al DB. La @ silenzia il warning: l'errore lo gestiamo noi sotto.
$mysqli = @new mysqli($DB_HOST, $DB_USER, $DB_PASS, $DB_NAME);
if ($mysqli->connect_errno) {
  http_response_code(500);                                  // 500 = errore lato server
  echo json_encode(['ok' => false, 'err' => 'db']);
  exit;
}
$mysqli->set_charset('utf8mb4');                            // codifica coerente con la tabella

// Costruzione della query: dati grezzi oppure medie per "bucket".
// I nomi sono rinominati (AS) in inglese corto: piu' comodi da usare nel JS.
if ($bucket === null) {
  // 24h: prendiamo le righe cosi' come sono, ordinate dal piu' vecchio al piu' nuovo.
  $sql = "SELECT ts AS t,
                 temperatura AS temp,
                 umidita     AS hum,
                 pressione   AS pres,
                 luce        AS luce
          FROM letture_meteo
          WHERE $where
          ORDER BY ts ASC";
} else {
  // Intervalli lunghi: una riga per fascia (bucket), con la MEDIA dei valori.
  // ROUND tiene i decimali "umani" (1 per i sensori, 0 per la luce intera).
  $sql = "SELECT MIN(ts) AS t,
                 ROUND(AVG(temperatura), 1) AS temp,
                 ROUND(AVG(umidita), 1)     AS hum,
                 ROUND(AVG(pressione), 1)   AS pres,
                 ROUND(AVG(luce))           AS luce
          FROM letture_meteo
          WHERE $where
          GROUP BY $bucket
          ORDER BY t ASC";
}

// Eseguiamo la query e travasiamo le righe in un array PHP.
// Convertiamo esplicitamente i numeri: dal DB arrivano come stringhe, ma nel
// JSON vogliamo veri numeri (cosi' Chart.js li disegna senza conversioni).
$points = [];
if ($res = $mysqli->query($sql)) {
  while ($row = $res->fetch_assoc()) {
    $points[] = [
      't'    => $row['t'],                                              // istante (testo "YYYY-MM-DD HH:MM:SS")
      'temp' => $row['temp'] === null ? null : (float) $row['temp'],    // null se la media non esiste
      'hum'  => $row['hum']  === null ? null : (float) $row['hum'],
      'pres' => $row['pres'] === null ? null : (float) $row['pres'],
      'luce' => $row['luce'] === null ? null : (int) $row['luce'],
    ];
  }
  $res->free();                                              // libera il risultato
}

// Ultima lettura reale (la riga col più alto id): serve per i "valori attuali"
// mostrati nelle card in cima alla pagina, indipendenti dall'intervallo scelto.
$latest = null;
if ($res = $mysqli->query("SELECT ts AS t, temperatura AS temp, umidita AS hum,
                                  pressione AS pres, luce AS luce
                           FROM letture_meteo ORDER BY id DESC LIMIT 1")) {
  if ($row = $res->fetch_assoc()) {
    $latest = [
      't'    => $row['t'],
      'temp' => (float) $row['temp'],
      'hum'  => (float) $row['hum'],
      'pres' => (float) $row['pres'],
      'luce' => (int)   $row['luce'],
    ];
  }
  $res->free();
}

$mysqli->close();                                            // chiude la connessione al DB

// Componiamo la risposta finale e la stampiamo come JSON: questo testo e'
// esattamente cio' che riceve fetch() in index.html.
echo json_encode([
  'ok'     => true,                                          // esito ok (il JS controlla questo campo)
  'range'  => $range,                                        // intervallo effettivamente servito
  'count'  => count($points),                               // quanti punti (utile per debug/stato)
  'latest' => $latest,                                       // ultima lettura reale (per le card)
  'points' => $points,                                       // serie di punti per i grafici
]);
