SELECT name
FROM ducklings
WHERE color = 'yellow'
  AND age >= 3
ORDER BY name ASC
LIMIT 2;
