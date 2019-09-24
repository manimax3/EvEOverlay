#include "db.h"
#include "logging.h"

#include <sqlite3.h>
#include <thread>

std::shared_ptr<sqlite3> eo::db::make_database_connection(const std::string &file)
{
    sqlite3 *db;
    if (sqlite3_open_v2(file.c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr) != SQLITE_OK) {
        throw std::runtime_error("Could not create/open database");
    }

    return std::shared_ptr<sqlite3>{ db, [](auto *ptr) {
                                        while (sqlite3_close(ptr) == SQLITE_BUSY) {
                                            std::this_thread::sleep_for(std::chrono::milliseconds(10));
                                        }
                                    } };
}

std::shared_ptr<sqlite3_stmt> eo::db::make_statement(std::shared_ptr<sqlite3> dbconnection, const std::string &stmt)
{
    sqlite3_stmt *sqlstmt;
    sqlite3_prepare_v2(dbconnection.get(), stmt.c_str(), stmt.length() + 1, &sqlstmt, nullptr);
    return std::shared_ptr<sqlite3_stmt>{ sqlstmt, [db = std::move(dbconnection)](auto *ptr) { sqlite3_finalize(ptr); } };
}

int eo::db::get_pragma_version(sqlite3 &dbconnection)
{
    int output;
    sqlite3_exec(
        &dbconnection, "PRAGMA user_version;",
        [](void *ud, int column_count, char **column_text, char **column_name) {
            int *output = static_cast<int *>(ud);
            *output     = std::atoi(column_text[0]);
            return 0;
        },
        &output, nullptr);

    return output;
}

void eo::db::set_pragma_version(sqlite3 &dbconnection, int value)
{
    const auto stmt = fmt::format("PRAGMA user_version = {0};", value);
    sqlite3_exec(&dbconnection, stmt.c_str(), nullptr, nullptr, nullptr);
}

void eo::db::create_tables(sqlite3 &dbconnection)
{
    const std::array statements = { "CREATE TABLE IF NOT EXISTS token(refreshtoken, charactername, characterid, accesstoken, expireson)" };

    for (const auto &stmt : statements) {
        sqlite3_exec(&dbconnection, stmt, nullptr, nullptr, nullptr);
    }
}
