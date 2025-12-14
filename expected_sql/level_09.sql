SELECT color, COUNT(*) AS count
FROM ducklings
GROUP BY color
HAVING COUNT(*) >= 3
ORDER BY count DESC, color ASC;
