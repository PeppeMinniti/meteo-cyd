#pragma once
// ============================================================================
//  SECRETS (template) - copia questo file in "secrets.h" e metti i tuoi valori.
//  secrets.h è ignorato da git e non finisce mai nel repo pubblico.
//
//    cp src/secrets.example.h src/secrets.h   (poi modifica secrets.h)
// ============================================================================

#define WIFI_SSID   "LA_TUA_RETE_WIFI"                              // Nome della rete WiFi
#define WIFI_PASS   "LA_TUA_PASSWORD_WIFI"                          // Password della rete WiFi
#define DB_ENDPOINT "https://www.tuosito.it/meteo/insert.php"       // URL HTTPS finale dell'insert.php (con www., niente 301)
#define DB_TOKEN    "CAMBIA_QUESTO_TOKEN_LUNGO_E_CASUALE"           // Deve combaciare con $SECRET in insert.php
