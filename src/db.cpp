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

#include "db.h"
#include "authentication.h"
#include "logging.h"

#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sqlite3.h>
#include <thread>
#include <zlib.h>

using json = nlohmann::json;

namespace {
json parse_invtypes_assetfile();
}

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
        sqlite3_exec(&dbconnection,
                     "CREATE TABLE IF NOT EXISTS token(refreshtoken, charactername, characterid, accesstoken, expireson, codechallenge)",
                     nullptr, nullptr, nullptr);

        break;
    case 1:
        sqlite3_exec(&dbconnection,
                     "CREATE TABLE IF NOT EXISTS solarsystem(id, constellationid, name, planets, position, secclass, secstatus, starid, "
                     "stargates, stations)",
                     nullptr, nullptr, nullptr);
        break;
    case 2:
        sqlite3_exec(&dbconnection, "CREATE TABLE IF NOT EXISTS killmail(id, hash, systemid, attackers, victim);", nullptr, nullptr,
                     nullptr);
        break;
    case 3: {
        sqlite3_exec(&dbconnection,
                     "CREATE TABLE IF NOT EXISTS invTypes(typeid, groupid, typename, description, mass, volume, capacity, portionsize, "
                     "raceid, baseprice, published, marketgroupid, iconid, soundid, graphicid);",
                     nullptr, nullptr, nullptr);
        log::info("Parsing invTypes sde data file. This might take a moment!");
        const auto result = parse_invtypes_assetfile();
        log::info("Inserting invTypes data into the database");

        // bad api desing from my side, so we have to do it manually
        sqlite3_stmt *begin_trans;
        sqlite3_prepare_v2(&dbconnection, "BEGIN TRANSACTION;", -1, &begin_trans, nullptr);
		sqlite3_step(begin_trans);
		sqlite3_finalize(begin_trans);
        sqlite3_stmt *sqlstmt;
        sqlite3_prepare_v2(&dbconnection, "INSERT INTO invTypes VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)", -1, &sqlstmt, nullptr);
        for (const auto &ie : result) {
            int   col    = 0;
            int32 ivalue = ie.at("typeID");
            sqlite3_bind_int(sqlstmt, ++col, ivalue);

            std::string_view value = ie.at("groupID").get<std::string_view>();
            sqlite3_bind_text(sqlstmt, ++col, value.data(), value.length(), nullptr);

            value = ie.at("typeName").get<std::string_view>();
            sqlite3_bind_text(sqlstmt, ++col, value.data(), value.length(), nullptr);
            value = ie.at("description").get<std::string_view>();
            sqlite3_bind_text(sqlstmt, ++col, value.data(), value.length(), nullptr);
            value = ie.at("mass").get<std::string_view>();
            sqlite3_bind_text(sqlstmt, ++col, value.data(), value.length(), nullptr);
            value = ie.at("volume").get<std::string_view>();
            sqlite3_bind_text(sqlstmt, ++col, value.data(), value.length(), nullptr);
            value = ie.at("capacity").get<std::string_view>();
            sqlite3_bind_text(sqlstmt, ++col, value.data(), value.length(), nullptr);
            value = ie.at("portionSize").get<std::string_view>();
            sqlite3_bind_text(sqlstmt, ++col, value.data(), value.length(), nullptr);
            value = ie.at("raceID").get<std::string_view>();
            sqlite3_bind_text(sqlstmt, ++col, value.data(), value.length(), nullptr);
            value = ie.at("basePrice").get<std::string_view>();
            sqlite3_bind_text(sqlstmt, ++col, value.data(), value.length(), nullptr);
            value = ie.at("published").get<std::string_view>();
            sqlite3_bind_text(sqlstmt, ++col, value.data(), value.length(), nullptr);
            value = ie.at("marketGroupID").get<std::string_view>();
            sqlite3_bind_text(sqlstmt, ++col, value.data(), value.length(), nullptr);
            value = ie.at("iconID").get<std::string_view>();
            sqlite3_bind_text(sqlstmt, ++col, value.data(), value.length(), nullptr);
            value = ie.at("soundID").get<std::string_view>();
            sqlite3_bind_text(sqlstmt, ++col, value.data(), value.length(), nullptr);
            value = ie.at("graphicID").get<std::string_view>();
            sqlite3_bind_text(sqlstmt, ++col, value.data(), value.length(), nullptr);

			sqlite3_step(sqlstmt);
			sqlite3_reset(sqlstmt);
        }
        sqlite3_finalize(sqlstmt);

        sqlite3_stmt *end_trans;
        sqlite3_prepare_v2(&dbconnection, "END TRANSACTION;", -1, &end_trans, nullptr);
		sqlite3_step(end_trans);
		sqlite3_finalize(end_trans);

    } break;
    default:
        throw std::logic_error(fmt::format("Unsupported database migration. from version {0} to version {1}", from, to));
    }

    set_pragma_version(dbconnection, to);
}

void eo::db::store_in_db(SqliteSPtr dbconnection, const TokenData &data)
{
    auto stmt = make_statement(std::move(dbconnection), "INSERT INTO token VALUES(?,?,?,?,?,?)");
    sqlite3_bind_text(stmt.get(), 1, data.refreshToken.c_str(), data.refreshToken.length(), nullptr);
    sqlite3_bind_text(stmt.get(), 2, data.characterName.c_str(), data.characterName.length(), nullptr);
    sqlite3_bind_int(stmt.get(), 3, data.characterID);
    sqlite3_bind_text(stmt.get(), 4, data.accessToken.c_str(), data.accessToken.length(), nullptr);
    sqlite3_bind_text(stmt.get(), 5, data.expiresOn.c_str(), data.expiresOn.length(), nullptr);
    sqlite3_bind_text(stmt.get(), 6, data.codeChallenge.c_str(), data.codeChallenge.length(), nullptr);
    sqlite3_step(stmt.get());
}

eo::TokenData eo::db::get_latest_tokendata_by_expiredate(SqliteSPtr dbconnection)
{
    auto countstmt = make_statement(dbconnection, "SELECT COUNT(*) FROM token ORDER BY expireson DESC LIMIT 1");

    sqlite3_step(countstmt.get());
    if (sqlite3_column_int(countstmt.get(), 0) == 0) {
        throw std::runtime_error("Could not find a token in the database");
    }

    auto stmt = make_statement(std::move(dbconnection), "SELECT * FROM token ORDER BY expireson DESC LIMIT 1");
    sqlite3_step(stmt.get());

    TokenData data;
    data.refreshToken  = column_get_string(stmt.get(), 0);
    data.characterName = column_get_string(stmt.get(), 1);
    data.characterID   = sqlite3_column_int(stmt.get(), 2);
    data.accessToken   = column_get_string(stmt.get(), 3);
    data.expiresOn     = column_get_string(stmt.get(), 4);
    data.codeChallenge = column_get_string(stmt.get(), 5);

    return data;
}

std::string eo::db::column_get_string(sqlite3_stmt *stmt, int col)
{
    std::string output;
    output.resize(sqlite3_column_bytes(stmt, col));
    std::memcpy(output.data(), sqlite3_column_text(stmt, col), output.length());
    return output;
}

namespace {
json parse_invtypes_assetfile()
{
    // Read the file
    std::ifstream ifs("assets/invTypes.json.zz", std::ios::binary | std::ios::ate);
    const auto    size = ifs.tellg();
    ifs.seekg(0, std::ios::beg);
    std::vector<char> buffer(size);
    ifs.read(buffer.data(), size);

    // Decompress it
    constexpr std::size_t chunksize = 64000;
    std::vector<uint8_t>  output_buffer;

    {
        std::array<uint8_t, chunksize> output;
        z_stream                       stream;
        stream.zalloc = Z_NULL;
        stream.zfree  = Z_NULL;
        stream.opaque = Z_NULL;
        inflateInit(&stream);
        stream.next_in  = reinterpret_cast<uint8_t *>(buffer.data());
        stream.avail_in = size;
        int ret         = 0;
        do {
            stream.next_out  = output.data();
            stream.avail_out = chunksize;
            ret              = inflate(&stream, Z_NO_FLUSH);
            output_buffer.insert(end(output_buffer), begin(output), begin(output) + (chunksize - stream.avail_out));
        } while (ret != Z_STREAM_END); // until no more output available

        inflateEnd(&stream);
    }

    return json::parse(output_buffer);
}
}
