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

constexpr const int CURRENT_VERSION = 2;

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
