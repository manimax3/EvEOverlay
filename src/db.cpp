#include "db.h"
#include "authentication.h"
#include "logging.h"

#include <sqlite3.h>
#include <thread>

std::shared_ptr<sqlite3> eo::db::make_database_connection(const std::string &file, bool migrate)
{
    sqlite3 *db;
    if (sqlite3_open_v2(file.c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr) != SQLITE_OK) {
        throw std::runtime_error("Could not create/open database");
    }

    if (migrate) {
        migrate_tables(*db, get_pragma_version(*db), CURRENT_VERSION);
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

void eo::db::migrate_tables(sqlite3 &dbconnection, int from, int to)
{
    if (from == to) {
        return; // No work required
    } else if (to - from > 1) {
        migrate_tables(dbconnection, from, to - 1);
        from = to - 1;
    } else if (to < from) {
        throw std::logic_error("migrate_tables: to is smaller than from value\n"
                               "The database was probably created by a newer version");
    }

    /*
     * If db schema changes:
     * 1. Bump version number in header
     * 2. Write the conversion in the switch below
     */

    switch (from) {
    case 0:
        sqlite3_exec(&dbconnection, "CREATE TABLE IF NOT EXISTS token(refreshtoken, charactername, characterid, accesstoken, expireson)",
                     nullptr, nullptr, nullptr);

        break;
    default:
        throw std::logic_error(fmt::format("Unsupported database migration. from version {0} to version {1}", from, to));
    }

    set_pragma_version(dbconnection, to);
}

void eo::db::store_in_db(SqliteSPtr dbconnection, const TokenData &data)
{
    auto stmt = make_statement(std::move(dbconnection), "INSERT INTO token VALUES(?,?,?,?,?)");
    sqlite3_bind_text(stmt.get(), 1, data.refreshToken.c_str(), data.refreshToken.length(), nullptr);
    sqlite3_bind_text(stmt.get(), 2, data.characterName.c_str(), data.characterName.length(), nullptr);
    sqlite3_bind_int(stmt.get(), 3, data.characterID);
    sqlite3_bind_text(stmt.get(), 4, data.accessToken.c_str(), data.accessToken.length(), nullptr);
    sqlite3_bind_text(stmt.get(), 5, data.expiresOn.c_str(), data.expiresOn.length(), nullptr);
    sqlite3_step(stmt.get());
}

eo::TokenData eo::db::get_latest_tokendata_by_expiredate(SqliteSPtr dbconnection)
{
    TokenData data;
    auto      stmt = make_statement(std::move(dbconnection), "SELECT * FROM token ORDER BY expireson DESC LIMIT 1");
    sqlite3_step(stmt.get());
    data.refreshToken  = reinterpret_cast<const char *>(sqlite3_column_text(stmt.get(), 0));
    data.characterName = reinterpret_cast<const char *>(sqlite3_column_text(stmt.get(), 1));
    data.characterID   = sqlite3_column_int(stmt.get(), 2);
    data.accessToken   = reinterpret_cast<const char *>(sqlite3_column_text(stmt.get(), 3));
    data.expiresOn     = reinterpret_cast<const char *>(sqlite3_column_text(stmt.get(), 4));

    return data;
}

