#pragma once

namespace database
{
    enum class database_type
    {
        sqlite3 = 0
    };

    void remove_inactive_players();
    nlohmann::json get_random_target_list(unsigned count);
    void add_player(uint64_t xuid, uint32_t player_id);
    void add_motherbase(uint32_t owner_player_id, uint32_t motherbase_id, uint32_t area_id, uint32_t construct_param, uint32_t platform_count, uint32_t security_rank);
    void add_ranking(uint32_t player_id, uint32_t fob_grade, uint32_t fob_point, uint32_t fob_rank);
    void initialize(database_type type);
}