# duckdb_dojo (extension-ready slice v0.3)

This ZIP contains the **extension code + SQL tests** for a DuckDB community extension that implements a mini SQL kata / “dojo” inside DuckDB.

## What’s included

- `src/dojo_extension.cpp` – task registry, hint retrieval, and check engine
- `src/include/dojo_extension.hpp` – extension class definition
- `test/sql/dojo_levels.test` – sqllogictest tests for Levels 1–12
- `spec/` – the ducklings dataset + task metadata + canonical expected SQL (for reference)

## Implemented SQL surface

- `dojo_setup()` – table function that creates the TEMP practice dataset `ducklings` in your session
- `dojo_tasks()` – table function listing tasks + metadata
- `dojo_hint(task_id, hint_level)` – scalar function returning progressive hints (1-based)
- `dojo_check(task_id, user_sql)` – table function that runs the user query and compares it to the expected output

`dojo_check` switches between **ordered** and **unordered** comparison based on each task’s `requires_order`.

## Notes

- The check engine resets the `ducklings` TEMP table on every `dojo_check` call using the `spec/ducklings.sql` contents. This keeps checks deterministic.
- Levels that don’t explicitly say “Sort by …” are configured with `requires_order=false` (unordered comparison).

## Using this with DuckDB’s extension template

1. Create a repo from the DuckDB extension template:
   - https://github.com/duckdb/extension-template
2. Rename the extension to `dojo`:
   - `python3 ./scripts/bootstrap-template.py dojo`
3. Copy this ZIP’s `src/*` and `test/sql/*` into your template repo, replacing the example `quack` code.
4. Build and run tests:
   - `make`
   - `make test`

If you prefer a different extension name (e.g. `duckdb_dojo`), run the bootstrap script with that name and rename function entrypoints accordingly.
