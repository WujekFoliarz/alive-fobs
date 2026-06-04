#include <pch.hpp>
#include <scan.hpp>
#include "database.hpp"

namespace scan
{
    static constexpr int START_PLAYER = 3996912;
    static constexpr int END_PLAYER = 3800000;
    static constexpr int MAX_REQUEST_POOL_THREADS = 4;
    static constexpr std::size_t MAX_REQUEST_QUEUE = 1024;
    static constexpr int BATCH_SIZE = 10;

    std::string STEAM_API_KEY = "None";
    std::jthread scan_thread;
    std::jthread live_player_scan_thread;
    BS::thread_pool request_pool;
    std::atomic<bool> running = false;
    bool initialized = false;
    constexpr std::string_view get_player_param = "/tppstm/get_player?player_id={}";
    constexpr std::string_view steam_get_player_summaries_param = "/ISteamUser/GetPlayerSummaries/v2/?key={}&steamids={}";

    std::vector<int64_t> is_steam_player_playing(std::vector<uint64_t> xuids)
    {
        std::vector<int64_t> playing_players;
        thread_local httplib::Client http_steam_api("http://api.steampowered.com");

        std::string steam_ids_param = "";

        for (auto &xuid : xuids)
        {
            steam_ids_param += std::to_string(xuid) + ",";
        }

        std::string request_url = std::format(steam_get_player_summaries_param, STEAM_API_KEY, steam_ids_param);
        auto result = http_steam_api.Get(request_url);

        if (!result)
        {
            std::cout << "is_steam_player_playing failed" << std::endl;
            return playing_players;
        }

        if (result->status != 200)
        {
            std::cout << "is_steam_player_playing failed, CODE: " << result->status << std::endl;
            return playing_players;
        }

        nlohmann::json json;
        try
        {
            json = nlohmann::json::parse(result->body);
        }
        catch (std::exception &e)
        {
            std::cout << result->body << std::endl;
            std::cout << "Failed to parse steam player json\n"
                      << e.what() << std::endl;
            return playing_players;
        }

        auto players = json["response"]["players"];

        for (auto &player : players)
        {
            if (player.empty())
            {
                continue;
            }

            std::string str_steamid = player["steamid"];
            int64_t steamid = 0;

            auto [ptr, ec] = std::from_chars(str_steamid.data(), str_steamid.data() + str_steamid.size(), steamid);

            if (ec != std::errc())
            {
                std::cout << "Conversion failed" << std::endl;
            }

            // bool is_online = player["personastate"] != 0 ? true : false;
            bool is_playing_mgsv = player["gameid"] == "287700" ? true : false;

            if (is_playing_mgsv)
            {
                playing_players.push_back(steamid);
                continue;
            }
        }

        return playing_players;
    }

    void request_worker(int player)
    {
        thread_local httplib::Client http_player_database("http://tpp-db.alicent.cat");
        http_player_database.set_follow_location(true);

        std::unordered_map<uint32_t, uint64_t> player_id_to_xuid;
        std::unordered_map<uint64_t, uint32_t> xuid_to_player_id;
        std::vector<uint64_t> steam_player_ids;

        for (int i = 0; i < BATCH_SIZE; i++)
        {
            std::string request_url = std::format(get_player_param, player);
            auto result = http_player_database.Get(request_url);

            if (!result)
            {
                std::cout << "Player request failed" << std::endl;
                continue;
            }

            if (result->status != 200)
            {
                std::cout << "Player request failed, CODE: " << result->status << std::endl;
                continue;
            }

            nlohmann::json json;
            try
            {
                json = nlohmann::json::parse(result->body);
            }
            catch (std::exception &e)
            {
                std::cout << "Failed to parse player (" << player << ") json\n"
                          << e.what() << std::endl;
                continue;
            }

            int player_id = json["player_id"];
            player--;

            if (json["mother_base_param"].empty())
            {
                continue;
            }

            int64_t xuid = json["xuid"];
            player_id_to_xuid[player_id] = xuid;
            xuid_to_player_id[xuid] = player_id;
            steam_player_ids.push_back(xuid);
        }

        if (steam_player_ids.empty())
        {
            return;
        }

        auto result = is_steam_player_playing(steam_player_ids);

        for (auto &player : result)
        {
            database::add_player(player, xuid_to_player_id[player]);
        }
    }

    void scan_worker(std::stop_token st)
    {
        static auto last_time_removed = std::chrono::steady_clock::now();
        int current_player_id = START_PLAYER;
        while (!st.stop_requested())
        {
            if (current_player_id < END_PLAYER)
            {
                current_player_id = START_PLAYER;
            }

            if (request_pool.get_tasks_queued() >= MAX_REQUEST_QUEUE)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                continue;
            }

            request_pool.detach_task([current_player_id]()
                                     { request_worker(current_player_id); });

            // Remove inactive players every 10 minutes
            if ((std::chrono::steady_clock::now() - last_time_removed) > std::chrono::minutes(10))
            {
                database::remove_inactive_players();
                last_time_removed = std::chrono::steady_clock::now();
                std::cout << "Cleaned up inactive players" << std::endl;
            }

            current_player_id -= BATCH_SIZE;
        }
    }

    void initialize(const std::string& steam_api_key)
    {
        if (initialized)
        {
            return;
        }

        request_pool.reset(MAX_REQUEST_POOL_THREADS);
        scan_thread = std::jthread(scan_worker);
        STEAM_API_KEY = steam_api_key;

        initialized = true;
    }
}