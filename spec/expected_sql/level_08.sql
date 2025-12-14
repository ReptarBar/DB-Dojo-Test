SELECT color, COUNT(*) AS count
FROM ducklings
GROUP BY color
ORDER BY count DESC, color ASC;
