SELECT
  name,
  age,
  ROW_NUMBER() OVER (ORDER BY age ASC, name ASC) AS age_rank
FROM ducklings
ORDER BY age_rank ASC, name ASC;
