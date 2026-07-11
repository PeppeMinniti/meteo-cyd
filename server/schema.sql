-- Tabella delle letture meteo da eseguire su phpMyAdmin (nel tuo database)
-- Esegui questo script una volta sola nella scheda "SQL" di phpMyAdmin.
--
-- NOTA DIDATTICA: una "tabella" e' come un foglio elettronico. Ogni RIGA e'
-- una misura inviata dall'ESP32; ogni COLONNA un dato (ora, temperatura...).
-- Scegliere il tipo giusto per colonna fa risparmiare spazio ed evita errori:
--   DECIMAL(5,1) = numero con 1 cifra decimale (es. 29.7); TINYINT UNSIGNED =
--   intero 0..255 (basta per una percentuale). L'INDICE su ts rende veloci le
--   query per data (quelle che fa data.php per i grafici).

CREATE TABLE IF NOT EXISTS letture_meteo (
  id          INT UNSIGNED NOT NULL AUTO_INCREMENT,        -- chiave progressiva
  ts          DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP, -- data/ora inserimento
  temperatura DECIMAL(5,1) NOT NULL,                       -- gradi C (es. 29.7)
  umidita     DECIMAL(5,1) NOT NULL,                       -- % (es. 56.3)
  pressione   DECIMAL(6,1) NOT NULL,                       -- hPa (es. 1016.8)
  luce        TINYINT UNSIGNED NOT NULL,                   -- % luce 0..100
  PRIMARY KEY (id),
  INDEX idx_ts (ts)                                        -- per query/grafici per data
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
