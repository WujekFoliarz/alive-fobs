#include "database.hpp"
#include <pch.hpp>
#include "db_schemas/motherbase_data.hpp"
#include "db_schemas/player_data.hpp"
#include "db_schemas/player_ranking_data.hpp"

namespace database
{
    bool initialized = false;
    static constexpr std::string_view db_filepath = "database/active_players.db";
    sqlpp::sqlite3::connection sqlite3_db;
    table::Players player_data_table;
    table::Motherbases motherbase_data_table;
    table::Rankings rankings_table;
    std::mutex db_lock;
    static constexpr int MAX_OFFLINE_DAYS = 7;

    void remove_inactive_players()
    {
        auto now = std::chrono::steady_clock::now();
        auto cutoff = std::chrono::system_clock::now() - std::chrono::hours{24 * MAX_OFFLINE_DAYS};

        std::lock_guard<std::mutex> lock(db_lock);

        sqlite3_db(delete_from(player_data_table)
                       .where(player_data_table.lastTimeOnline < cutoff));
    }

    nlohmann::json get_random_target_list(unsigned count)
    {
        thread_local httplib::Client http_player_database("http://tpp-db.alicent.cat");
        http_player_database.set_follow_location(true);
        auto cutoff = std::chrono::system_clock::now() - std::chrono::hours{24 * MAX_OFFLINE_DAYS};

        auto with_db_lock = [&](auto&& fn) {
            std::lock_guard<std::mutex> lock(db_lock);
            return fn();
        };

        auto result = with_db_lock([&] {
            return sqlite3_db(
                select(all_of(player_data_table))
                    .from(player_data_table)
                    .where(player_data_table.lastTimeOnline > cutoff)
                << sqlpp::verbatim_clause("ORDER BY RANDOM()")
                << sqlpp::limit(count)
            );
        });

        nlohmann::json json = nlohmann::json::array();
        for (const auto &row : result)
        {
            auto result_from_db_motherbase = with_db_lock([&] {
                return sqlite3_db(
                select(all_of(motherbase_data_table))
                    .from(motherbase_data_table)
                    .where(motherbase_data_table.ownerPlayerId == row.playerId) 
                );
            });


            nlohmann::json response_json;
            std::string get_player_request = "/tppstm/get_player?player_id=" + std::to_string(static_cast<uint32_t>(row.playerId.value()));
            nlohmann::json player_json = nlohmann::json::object();
            if (result_from_db_motherbase.empty())
            {
                if (auto player_res = http_player_database.Get(get_player_request))
                {
                    const auto &response = player_res->body;
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
                        auto &player_mb = player_json["mother_base_param"][str_num];
                        player_mb["mother_base_id"] = mother_base["mother_base_param"]["mother_base_id"];
                        player_mb["construct_param"] = mother_base["mother_base_param"]["construct_param"];
                        player_mb["area_id"] = mother_base["mother_base_param"]["area_id"];
                        player_mb["security_rank"] = mother_base["mother_base_param"]["security_rank"];
                        player_mb["platform_count"] = mother_base["mother_base_param"]["platform_count"];
                        add_motherbase(row.playerId.value(), player_mb["mother_base_id"], player_mb["area_id"], player_mb["construct_param"], player_mb["platform_count"], player_mb["security_rank"]);
                        mb_num++;
                    }
                }
                else
                {
                    std::cout << "Failed to get player data in get_random_target_list" << std::endl;
                    continue;
                }
            }
            else
            {
                player_json["mother_base_param"] = nlohmann::json::object();
                int mb_num = 0;
                for (const auto &mb_row : result_from_db_motherbase)
                {
                    std::string str_num = std::to_string(mb_num);
                    auto &player_mb = player_json["mother_base_param"][str_num];
                    player_mb["mother_base_id"] = mb_row.motherbaseId;
                    player_mb["construct_param"] = mb_row.constructParam;
                    player_mb["area_id"] = mb_row.areaId;
                    player_mb["security_rank"] = mb_row.securityRank;
                    player_mb["platform_count"] = mb_row.platformCount;
                }
            }

            auto result_from_db_ranking = sqlite3_db(
                select(all_of(rankings_table))
                    .from(rankings_table)
                    .where(rankings_table.playerId == row.playerId)
            );

            if (result_from_db_ranking.empty())
            {
                if (response_json.empty())
                {
                    if (auto player_res = http_player_database.Get(get_player_request))
                    {
                        const auto &response = player_res->body;
                        if (nlohmann::json::accept(response))
                        {
                            response_json = nlohmann::json::parse(response);
                        }
                        else
                        {
                            std::cout << "failed to parse player response in get_random_target_list" << std::endl;
                            continue;
                        }
                    }
                }

                auto &player_ranking = player_json["ranking"];
                auto &response_ranking = response_json["ranking"];
                player_ranking["fob_grade"] = response_ranking["fob_grade"];
                player_ranking["fob_point"] = response_ranking["fob_point"];
                player_ranking["fob_rank"] = response_ranking["fob_rank"];
                add_ranking(row.playerId.value(), player_ranking["fob_grade"], player_ranking["fob_point"], player_ranking["fob_rank"]);
            }
            else
            {
                for(const auto& rnk_row : result_from_db_ranking)
                {
                    auto &player_ranking = player_json["ranking"];
                    player_ranking["fob_grade"] = rnk_row.fobGrade.value_or(0);
                    player_ranking["fob_point"] = rnk_row.fobPoint.value_or(0);
                    player_ranking["fob_rank"] = rnk_row.fobRank.value_or(0);
                }
            }


            player_json["xuid"] = row.xuid.value();
            player_json["player_id"] = row.playerId.value();

            json.push_back(player_json);
        }

        return json;
    }

    void add_player(uint64_t xuid, uint32_t player_id)
    {
        std::cout << "Added player " << player_id << std::endl;
        std::lock_guard<std::mutex> lock(db_lock);
        sqlite3_db(sqlpp::sqlite3::insert_into(player_data_table)
                       .set(player_data_table.xuid = xuid,
                            player_data_table.playerId = player_id,
                            player_data_table.lastTimeOnline = std::chrono::system_clock::now())
                        .on_conflict(player_data_table.playerId)
                        .do_update(
                            player_data_table.xuid = xuid,
                            player_data_table.lastTimeOnline = std::chrono::system_clock::now()
                        ));
    }

    void add_motherbase(uint32_t owner_player_id, uint32_t motherbase_id, uint32_t area_id, uint32_t construct_param, uint32_t platform_count, uint32_t security_rank)
    {
        std::cout << "Added motherbase " << motherbase_id<< std::endl;
        std::lock_guard<std::mutex> lock(db_lock);
        sqlite3_db(sqlpp::sqlite3::insert_into(motherbase_data_table)
                    .set(motherbase_data_table.motherbaseId = motherbase_id,
                         motherbase_data_table.ownerPlayerId = owner_player_id, 
                         motherbase_data_table.areaId = area_id,
                         motherbase_data_table.constructParam = construct_param,
                         motherbase_data_table.platformCount = platform_count,
                         motherbase_data_table.securityRank = security_rank)
                    .on_conflict(motherbase_data_table.motherbaseId)
                    .do_update(
                         motherbase_data_table.ownerPlayerId = owner_player_id,
                         motherbase_data_table.areaId = area_id,
                         motherbase_data_table.constructParam = construct_param,
                         motherbase_data_table.platformCount = platform_count,
                         motherbase_data_table.securityRank = security_rank
                    ));
    }

    void add_ranking(uint32_t player_id, uint32_t fob_grade, uint32_t fob_point, uint32_t fob_rank)
    {
        std::cout << "Added ranking for player " << player_id << std::endl;
        std::lock_guard<std::mutex> lock(db_lock);
        sqlite3_db(sqlpp::sqlite3::insert_into(rankings_table)
                    .set(rankings_table.playerId = player_id,
                        rankings_table.fobGrade = fob_grade,
                        rankings_table.fobPoint = fob_point,
                        rankings_table.fobRank = fob_rank)
                    .on_conflict(rankings_table.playerId)
                    .do_update(
                        rankings_table.fobGrade = fob_grade,
                        rankings_table.fobPoint = fob_point,
                        rankings_table.fobRank = fob_rank
                    ));
    }

    void initialize(database_type type)
    {
        if (initialized)
        {
            return;
        }

        std::filesystem::create_directory("database");
        std::lock_guard<std::mutex> lock(db_lock);

        switch (type)
        {
        case database_type::sqlite3:
        {
            auto config = std::make_shared<sqlpp::sqlite3::connection_config>();
            config->path_to_database = db_filepath;
            config->flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
            sqlite3_db = sqlpp::sqlite3::connection();
            sqlite3_db.connect_using(config);

            table::createPlayers(sqlite3_db);
            table::createMotherbases(sqlite3_db); 
            table::createRankings(sqlite3_db); 
                 
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