#pragma once
#include "util.h"
#include <memory>
#include <string_view>

extern "C" {
struct sqlite3;
struct sqlite3_stmt;
}

namespace eo::db {

constexpr const int CURRENT_VERSION = 1;

std::shared_ptr<sqlite3> make_database_connection(const std::string &file = get_exe_dir() + data_folder + "data.db", bool migrate = true);
std::shared_ptr<sqlite3_stmt> make_statement(std::shared_ptr<sqlite3> dbconnection, const std::string &stmt);

void migrate_tables(sqlite3 &dbconnection, int from, int to);
int  get_pragma_version(sqlite3 &dbconnection);
void set_pragma_version(sqlite3 &dbconnection, int value);
}
