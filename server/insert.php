<?php
// ============================================================================
//  insert.php - Ponte (API di SCRITTURA) tra l'ESP32 e il MySQL di Aruba.
//  File commentato a scopo didattico.
//
//  PERCHE' ESISTE
//  ---------------------------------------------------------------------------
//  Il MySQL condiviso di Aruba non accetta connessioni da internet: l'ESP32
//  non puo' scriverci direttamente. Allora l'ESP32 chiama questo PHP (che gira
//  sullo spazio web Aruba, quindi "vicino" al DB) e gli passa i dati; il PHP
//  li inserisce nella tabella. Questo file e' il GEMELLO di data.php:
//  insert.php SCRIVE (lo usa l'ESP32), data.php LEGGE (la usa la dashboard).
//
//  TEORIA - concetti chiave
//  ---------------------------------------------------------------------------
//  - POST: metodo HTTP per INVIARE dati al server (a differenza della GET, che
//    serve a chiederli). L'ESP32 manda i campi nel corpo della richiesta.
//  - TOKEN condiviso: una "parola d'ordine" (qui $SECRET) nota solo a ESP32 e
//    a questo script. Senza il token giusto la richiesta viene rifiutata: e'
//    un'autenticazione minima per evitare che chiunque scriva nel DB.
//  - PREPARED STATEMENT: si invia a MySQL la query con dei segnaposto (?) e
//    SEPARATAMENTE i valori. Cosi' i dati non possono "diventare" comandi SQL:
//    e' la difesa standard contro la SQL injection.
//
//  Carica questo file nel tuo spazio web Aruba (es. /meteo/insert.php).
//  L'ESP32 ci fa una POST con: token, temp, hum, pres, light.
// ============================================================================

// --- CONFIGURAZIONE: credenziali in secrets.php (NON nel repo) --------------
// Copia secrets.example.php in secrets.php e inserisci i tuoi dati Aruba.
// secrets.php definisce: $DB_HOST, $DB_NAME, $DB_USER, $DB_PASS, $SECRET
// ($SECRET = token condiviso con l'ESP32, deve combaciare con DB_TOKEN).
require __DIR__ . '/secrets.php';
// ---------------------------------------------------------------------------

header('Content-Type: application/json; charset=utf-8');

// Accetta solo richieste POST
if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
  http_response_code(405);
  echo json_encode(['ok' => false, 'err' => 'method']);
  exit;
}

// Verifica il token (autenticazione semplice)
if (!isset($_POST['token']) || !hash_equals($SECRET, $_POST['token'])) {
  http_response_code(401);
  echo json_encode(['ok' => false, 'err' => 'auth']);
  exit;
}

// Legge e valida i parametri numerici
$temp = $_POST['temp']  ?? null;
$hum  = $_POST['hum']   ?? null;
$pres = $_POST['pres']  ?? null;
$luce = $_POST['light'] ?? null;

if (!is_numeric($temp) || !is_numeric($hum) || !is_numeric($pres) || !is_numeric($luce)) {
  http_response_code(400);
  echo json_encode(['ok' => false, 'err' => 'params']);
  exit;
}

// Connessione al DB
$mysqli = @new mysqli($DB_HOST, $DB_USER, $DB_PASS, $DB_NAME);
if ($mysqli->connect_errno) {
  http_response_code(500);
  echo json_encode(['ok' => false, 'err' => 'db', 'detail' => $mysqli->connect_error]);
  exit;
}

// Inserimento con prepared statement (sicuro contro SQL injection)
$stmt = $mysqli->prepare(
  'INSERT INTO letture_meteo (temperatura, umidita, pressione, luce) VALUES (?, ?, ?, ?)'
);
// d = double (temp/hum/pres), i = intero (luce)
$luceInt = (int) round($luce);
$stmt->bind_param('dddi', $temp, $hum, $pres, $luceInt);

if ($stmt->execute()) {
  echo json_encode(['ok' => true, 'id' => $stmt->insert_id]);
} else {
  http_response_code(500);
  echo json_encode(['ok' => false, 'err' => 'insert', 'detail' => $stmt->error]);
}

$stmt->close();
$mysqli->close();
