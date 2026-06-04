#pragma once

#include "table.hpp"

namespace table
{
    namespace player_data
    {
        DEFINE_FIELD(id, sqlpp::integer_unsigned);
        DEFINE_FIELD(player_id, sqlpp::integer_unsigned);
        DEFINE_FIELD(xuid, sqlpp::bigint_unsigned);
        DEFINE_FIELD(last_time_online, sqlpp::time_point);
        DEFINE_TABLE(players, id_field_t, player_id_field_t, xuid_field_t, last_time_online_field_t);
    };
};