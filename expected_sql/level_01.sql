SELECT name
FROM ducklings
WHERE color = 'yellow'
  AND age < 5
ORDER BY age ASC, name ASC
LIMIT 3;
