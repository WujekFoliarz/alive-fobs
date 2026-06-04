#include <pch.hpp>
#include <scan.hpp>
#include "database.hpp"

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        std::cerr << "Usage: " << "<ip> <port>" << std::endl;
        return 1;
    }

    std::string ip = argv[1];
    int port = std::stoi(argv[2]);

    dotenv env(".env");  // Load variables from .env file
    std::string steam_api_key = env.get("STEAM_API_KEY", "None");

    if (steam_api_key == "None" || steam_api_key == "YOUR_STEAM_API_HERE")
    {
        std::cout << "Steam API key was not setup" << std::endl;
    }

    database::initialize(database::database_type::sqlite3);
    scan::initialize(steam_api_key);
    httplib::Server svr;

    svr.Get("/", [&](const httplib::Request &, httplib::Response &res)
    {
        auto result = database::get_random_target_list(10);
        res.set_content(result.dump(4), "text/json");
    });

    std::cout << "Started server" << std::endl;
    svr.listen(ip, port);
}