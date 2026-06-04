#pragma once
#include <thread>
#include <atomic>
#include <format>
#include <string>

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <httplib.h>
#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/sqlite3/sqlite3.h>
#include <nlohmann/json.hpp>
#include <BS_thread_pool.hpp>
#include "dotenv.h"