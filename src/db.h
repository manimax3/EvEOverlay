// Copyright 2019 Maximilian Schiller
/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once
#include "util.h"
#include <memory>
#include <string_view>

extern "C" {
struct sqlite3;
struct sqlite3_stmt;
}

namespace eo {
struct TokenData;
}

namespace eo::db {

constexpr const int CURRENT_VERSION = 4;

using SqliteSPtr     = std::shared_ptr<sqlite3>;
using SqliteStmtSPtr = std::shared_ptr<sqlite3_stmt>;

SqliteSPtr     make_database_connection(const std::string &file = get_exe_dir() + data_folder + "data.db", bool migrate = true);
SqliteStmtSPtr make_statement(SqliteSPtr dbconnection, const std::string &stmt);

std::string column_get_string(sqlite3_stmt *stmt, int col);

void migrate_tables(sqlite3 &dbconnection, int from, int to);
int  get_pragma_version(sqlite3 &dbconnection);
void set_pragma_version(sqlite3 &dbconnection, int value);

// Store, Load and other helper functions
void      store_in_db(SqliteSPtr dbconnection, const TokenData &data);
TokenData get_latest_tokendata_by_expiredate(SqliteSPtr dbconnection);
}
