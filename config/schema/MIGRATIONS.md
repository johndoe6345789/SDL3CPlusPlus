# Config Schema Migrations

This folder tracks schema versions and how to migrate config JSON between them.

## Version 2 (current)
- File: runtime_config_v2.schema.json
- Supports the current runtime config shape plus optional render-graph sections.

## Stub: v2 -> v3
When bumping to v3, add a migration step in `JsonConfigService::ApplyMigrations` that:
- Detects `schema_version` or `configVersion` == 2
- Transforms renamed or restructured fields into v3 layout
- Emits trace logging for each field transformation
- Updates `schema_version` and `configVersion` to 3

Add notes here for each structural change so the migration remains deterministic.
