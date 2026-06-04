#include "database.hpp"
#include <pch.hpp>
#include "player_data.hpp"

namespace database
{
    bool initialized = false;
    static constexpr std::string_view db_filepath = "database/active_players.db";
    sqlpp::sqlite3::connection sqlite3_db;
    table::player_data::table_t player_data_table;
    std::mutex db_lock;
    static constexpr int MAX_OFFLINE_DAYS = 7;

    void remove_inactive_players()
    {
        auto now = std::chrono::steady_clock::now();
        auto cutoff = std::chrono::system_clock::now() - std::chrono::hours{24 * MAX_OFFLINE_DAYS};

        std::lock_guard<std::mutex> lock(db_lock);
        sqlite3_db(sqlpp::remove_from(player_data_table)
                       .where(player_data_table.last_time_online < cutoff));
    }

    nlohmann::json get_random_target_list(unsigned count)
    {
        thread_local httplib::Client http_player_database("http://tpp-db.alicent.cat");
        http_player_database.set_follow_location(true);
        auto cutoff = std::chrono::system_clock::now() - std::chrono::hours{24 * MAX_OFFLINE_DAYS};

        std::lock_guard<std::mutex> lock(db_lock);
        auto result = sqlite3_db(
            sqlpp::custom_query(
                select(all_of(player_data_table))
                    .from(player_data_table)
                    .where(player_data_table.last_time_online > cutoff),
                sqlpp::verbatim("ORDER BY RANDOM()"), sqlpp::limit(count)));

        nlohmann::json json = nlohmann::json::array();
        for (const auto &row : result)
        {
            nlohmann::json player_json = nlohmann::json::object();
            std::string get_player_request = "/tppstm/get_player?player_id=" + std::to_string(static_cast<uint32_t>(row.player_id));

            if (auto player_res = http_player_database.Get(get_player_request))
            {
                const auto &response = player_res->body;
                nlohmann::json response_json;
                if (nlohmann::json::accept(response))
                {
                    response_json = nlohmann::json::parse(response);
                }
                else
                {
                    std::cout << "failed to parse player response in get_random_target_list" << std::endl;
                    continue;
                }

                if (!response_json.contains("mother_base_param") || response_json["mother_base_param"].empty())
                {
                    continue;
                }

                player_json["mother_base_param"] = nlohmann::json::object();
                int mb_num = 0;
                for (const auto &mother_base : response_json["mother_base_param"])
                {
                    std::string str_num = std::to_string(mb_num);
                    auto& player_mb = player_json["mother_base_param"][str_num];
                    player_mb["mother_base_id"] = mother_base["mother_base_param"]["mother_base_id"];
                    player_mb["construct_param"] = mother_base["mother_base_param"]["construct_param"];
                    player_mb["area_id"] = mother_base["mother_base_param"]["area_id"];
                    player_mb["security_rank"] = mother_base["mother_base_param"]["security_rank"];
                    player_mb["platform_count"] = mother_base["mother_base_param"]["platform_count"];
                    mb_num++;
                }

                auto& player_ranking = player_json["ranking"];
                auto& response_ranking = response_json["ranking"];
                player_ranking["fob_grade"] = response_ranking["fob_grade"];
                player_ranking["fob_point"] = response_ranking["fob_point"];
                player_ranking["fob_rank"] = response_ranking["fob_rank"];
            }
            else
            {
                std::cout << "Failed to get player data in get_random_target_list" << std::endl;
                continue;
            }

            player_json["xuid"] = static_cast<int64_t>(row.xuid);
            player_json["player_id"] = static_cast<int>(row.player_id);

            json.push_back(player_json);
        }

        return json;
    }

    void add_player(int64_t xuid, int32_t player_id)
    {
        std::cout << "Added player " << player_id << std::endl;
        std::lock_guard<std::mutex> lock(db_lock);
        sqlite3_db(insert_into(player_data_table)
                       .set(player_data_table.xuid = xuid,
                            player_data_table.player_id = player_id,
                            player_data_table.last_time_online = std::chrono::system_clock::now()));
    }

    void initialize(database_type type)
    {
        if (initialized)
        {
            return;
        }

        std::lock_guard<std::mutex> lock(db_lock);

        switch (type)
        {
        case database_type::sqlite3:
        {
            sqlpp::sqlite3::connection_config config;
            config.path_to_database = db_filepath;
            config.flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
            sqlite3_db = sqlpp::sqlite3::connection(config);

            sqlite3_db.execute(R"(
                CREATE TABLE IF NOT EXISTS players (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    player_id INTEGER UNSIGNED UNIQUE,
                    xuid BIGINT UNSIGNED UNIQUE,
                    last_time_online DATETIME
                )
                )");

            break;
        }
        default:
        {
            return;
        }
        }

        initialized = true;
    }
}