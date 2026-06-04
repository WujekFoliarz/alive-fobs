#include <pch.hpp>
#include <scan.hpp>
#include "database.hpp"

int main()
{
    dotenv env(".env");  // Load variables from .env file
    std::string steam_api_key = env.get("STEAM_API_KEY", "None");

    database::initialize(database::database_type::sqlite3);
    scan::initialize(steam_api_key);
    httplib::Server svr;

    svr.Get("/", [&](const httplib::Request &, httplib::Response &res)
    {
        auto result = database::get_random_target_list(10);
        res.set_content(result.dump(4), "text/json");
    });

    std::cout << "Started server" << std::endl;
    svr.listen("127.0.0.1", 8080);
}