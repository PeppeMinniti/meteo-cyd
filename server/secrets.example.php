<?php
// ============================================================================
//  secrets.example.php - TEMPLATE delle credenziali del server.
//  Copia questo file in "secrets.php" e inserisci i tuoi valori:
//
//      cp secrets.example.php secrets.php   (poi modifica secrets.php)
//
//  secrets.php è ignorato da git e non finisce mai nel repo pubblico.
//  Carica secrets.php accanto a insert.php / data.php sullo spazio web.
// ============================================================================

$DB_HOST = 'localhost';                            // Di solito 'localhost' su Aruba
$DB_NAME = 'IL_TUO_DATABASE';                       // Nome del database
$DB_USER = 'IL_TUO_UTENTE';                         // Utente MySQL
$DB_PASS = 'LA_TUA_PASSWORD_DB';                    // Password del database
$SECRET  = 'CAMBIA_QUESTO_TOKEN_LUNGO_E_CASUALE';   // Token: deve combaciare con DB_TOKEN nel firmware
