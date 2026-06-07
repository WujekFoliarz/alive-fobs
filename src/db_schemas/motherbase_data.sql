CREATE TABLE IF NOT EXISTS motherbases (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    motherbase_id INTEGER UNSIGNED UNIQUE,
    owner_player_id INTEGER UNSIGNED,
    area_id INTEGER UNSIGNED,
    construct_param INTEGER UNSIGNED,
    platform_count INTEGER UNSIGNED,
    security_rank INTEGER UNSIGNED
)