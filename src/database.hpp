#pragma once

namespace database
{
    enum class database_type
    {
        sqlite3 = 0
    };

    void remove_inactive_players();
    nlohmann::json get_random_target_list(unsigned count);
    void add_player(int64_t xuid, int32_t player_id);
    void initialize(database_type type);
}