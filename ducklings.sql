-- duckdb_dojo v0.1 starter dataset
-- Run this in your session before attempting dojo levels:
--   .read ducklings.sql
-- or copy/paste into DuckDB.
--
-- If we implement dojo_setup() in the extension, it will execute the equivalent.

CREATE OR REPLACE TEMP TABLE ducklings (
  name  VARCHAR,
  color VARCHAR,
  age   INTEGER
);

INSERT INTO ducklings (name, color, age) VALUES
  ('Daffy',   'yellow', 1),
  ('Goldie',  'yellow', 2),
  ('Daisy',   'yellow', 3),
  ('Puddles', 'brown',  4),
  ('Waddles', 'green',  5),
  ('Beakley', 'brown',  6),
  ('Mallow',  'green',  7),
  ('Nugget',  'yellow', 8),
  ('Duke',    'brown',  9),
  ('Splash',  'blue',  10),
  ('Moss',    'brown', 11),
  ('Sunny',   'yellow', 12);

-- Sanity check:
-- SELECT * FROM ducklings ORDER BY age;
