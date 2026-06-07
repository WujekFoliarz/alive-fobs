#pragma once
#include <thread>
#include <atomic>
#include <format>
#include <string>

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <httplib.h>
#include <sqlpp23/sqlpp23.h>
#include <sqlpp23/sqlite3/sqlite3.h>
#include <nlohmann/json.hpp>
#include <BS_thread_pool.hpp>
#include "dotenv.h"