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

#include "esisession.h"
#include "logging.h"
#include "requests.h"

#include <fmt/core.h>
#include <nlohmann/json.hpp>
#include <sqlite3.h>

using namespace eo::esi;
using json = nlohmann::json;

eo::EsiSession::EsiSession(const db::SqliteSPtr &mDbConnection, std::shared_ptr<IOState> iostate)
    : mDbConnection(mDbConnection)
    , mIOState(std::move(iostate))
{
    if (!mDbConnection) {
        throw std::logic_error("EsiSession requries a valid mDbConnection");
    }
    if (!mIOState) {
        throw std::logic_error("EsiSession requires a valid iostate");
    }

    TokenData token;
    // TODO Quite the common case, should not be handled by exception
    try {
        token = db::get_latest_tokendata_by_expiredate(mDbConnection);
        if (token_expired(token)) {
            refresh_token(token);
            db::store_in_db(mDbConnection, token);
        }
    } catch (const std::runtime_error &re) {
        log::error("{0}", re.what());
        log::info("Trying to retrieve a new auth token");
        const auto cc = make_authorize_request({ "esi-location.read_location.v1" });
        const auto ac = handle_redirect();
        const auto tr = make_token_request(ac, cc);
        const auto vr = verify_token(tr.access_token);
        token         = make_token_data(tr, vr);
        db::store_in_db(mDbConnection, token);
    }

    mCurrentToken = std::move(token);
}

CharacterLocation eo::EsiSession::getCharacterLocation()
{
    if (token_expired(mCurrentToken)) {
        refresh_token(mCurrentToken);
        db::store_in_db(mDbConnection, mCurrentToken);
    }

    HttpRequest request;
    request.hostname                            = "esi.evetech.net";
    request.target                              = fmt::format("/v1/characters/{0}/location/", mCurrentToken.characterID);
    request.headers[http::field::authorization] = fmt::format("Bearer {0}", mCurrentToken.accessToken);
    const auto response                         = makeHttpRequest(request);
    const auto j                                = json::parse(response.body);

    CharacterLocation location{};
    try {
        j.at("solar_system_id").get_to(location.solarSystemID);
    } catch (const json::out_of_range &e) {
        log::error("Missing solar system id for location retrieval");
        log::error("{0}", response.body);
        throw e;
    }

    // TODO Quite the common case, should not be handled by exception
    try {
        j.at("station_id").get_to(location.stationID);
        j.at("structure_id").get_to(location.structureID);
    } catch (const json::out_of_range &e) {
        // Did not contain any staion or structure information
    }

    return location;
}

void eo::EsiSession::getCharacterLocationAsync(std::function<void(const CharacterLocation &)> callback)
{
    if (token_expired(mCurrentToken)) {
        refresh_token(mCurrentToken);
        db::store_in_db(mDbConnection, mCurrentToken);
    }

    HttpRequest request;
    request.hostname                            = "esi.evetech.net";
    request.target                              = fmt::format("/v1/characters/{0}/location/", mCurrentToken.characterID);
    request.headers[http::field::authorization] = fmt::format("Bearer {0}", mCurrentToken.accessToken);

    mIOState->makeAsyncHttpRequest(request, [callback = std::move(callback)](auto &&response, auto &&) {
        const auto        j = json::parse(response.body);
        CharacterLocation location{};
        try {
            j.at("solar_system_id").get_to(location.solarSystemID);
        } catch (const json::out_of_range &e) {
            log::error("Missing solar system id for location retrieval");
            log::error("{0}", response.body);
            throw e;
        }

        // TODO Quite the common case, should not be handled by exception
        try {
            j.at("station_id").get_to(location.stationID);
            j.at("structure_id").get_to(location.structureID);
        } catch (const json::out_of_range &e) {
            // Did not contain any staion or structure information
        }

        callback(location);
    });
}

SolarSystem eo::EsiSession::resolveSolarSystem(int32 solarSystemID)
{
    SolarSystem system;
    auto        stmt = db::make_statement(mDbConnection, "SELECT COUNT(*) FROM solarsystem WHERE id = ?;");
    sqlite3_bind_int(stmt.get(), 1, solarSystemID);
    sqlite3_step(stmt.get());
    if (const auto results = sqlite3_column_int(stmt.get(), 0); results == 1) {
        auto select = db::make_statement(mDbConnection, "SELECT * FROM solarsystem WHERE id = ? LIMIT 1;");
        sqlite3_bind_int(select.get(), 1, solarSystemID);
        sqlite3_step(select.get());

        system.systemID        = solarSystemID;
        system.constellationID = sqlite3_column_int(select.get(), 1);
        system.name            = db::column_get_string(select.get(), 2);
        system.planetsJson     = db::column_get_string(select.get(), 3);
        system.positionJson    = db::column_get_string(select.get(), 4);
        system.securityClass   = db::column_get_string(select.get(), 5);
        system.securityStatus  = sqlite3_column_double(select.get(), 6);
        system.starID          = sqlite3_column_int(select.get(), 7);
        system.stargatesJson   = db::column_get_string(select.get(), 8);
        system.stationsJson    = db::column_get_string(select.get(), 9);
        return system;

    } else if (results == 0) {
        HttpRequest request;
        request.hostname = "esi.evetech.net";
        request.target   = fmt::format("/v4/universe/systems/{0}/", solarSystemID);

        const auto response = makeHttpRequest(request);
        const auto j        = json::parse(response.body);

        j.at("constellation_id").get_to(system.constellationID);
        j.at("name").get_to(system.name);
        system.planetsJson  = j.at("planets").dump();
        system.positionJson = j.at("position").dump();
        j.at("security_class").get_to(system.securityClass);
        j.at("security_status").get_to(system.securityStatus);
        j.at("star_id").get_to(system.starID);
        system.stargatesJson = j.at("stargates").dump();
        system.stationsJson  = j.at("stations").dump();
        j.at("system_id").get_to(system.systemID);

        assert(solarSystemID == system.systemID);
    } else {
        throw std::runtime_error(fmt::format("Found {0} solar systems with the id {1} in the database", results, solarSystemID));
    }

    // Store the system in the database
    stmt = db::make_statement(mDbConnection, "INSERT INTO solarsystem VALUES(?,?,?,?,?,?,?,?,?,?)");
    sqlite3_bind_int(stmt.get(), 1, system.systemID);
    sqlite3_bind_int(stmt.get(), 2, system.constellationID);
    sqlite3_bind_text(stmt.get(), 3, system.name.c_str(), -1, nullptr);
    sqlite3_bind_text(stmt.get(), 4, system.planetsJson.c_str(), -1, nullptr);
    sqlite3_bind_text(stmt.get(), 5, system.positionJson.c_str(), -1, nullptr);
    sqlite3_bind_text(stmt.get(), 6, system.securityClass.c_str(), -1, nullptr);
    sqlite3_bind_double(stmt.get(), 7, system.securityStatus);
    sqlite3_bind_int(stmt.get(), 8, system.starID);
    sqlite3_bind_text(stmt.get(), 9, system.stargatesJson.c_str(), -1, nullptr);
    sqlite3_bind_text(stmt.get(), 10, system.stationsJson.c_str(), -1, nullptr);
    sqlite3_step(stmt.get());

    return system;
}

void eo::EsiSession::resolveSolarSystemAsync(int32 solarSystemID, std::function<void(const esi::SolarSystem &)> callback)
{
    auto stmt = db::make_statement(mDbConnection, "SELECT COUNT(*) FROM solarsystem WHERE id = ?;");
    sqlite3_bind_int(stmt.get(), 1, solarSystemID);
    sqlite3_step(stmt.get());
    if (const auto results = sqlite3_column_int(stmt.get(), 0); results == 1) {
        SolarSystem system;
        auto        select = db::make_statement(mDbConnection, "SELECT * FROM solarsystem WHERE id = ? LIMIT 1;");
        sqlite3_bind_int(select.get(), 1, solarSystemID);
        sqlite3_step(select.get());

        system.systemID        = solarSystemID;
        system.constellationID = sqlite3_column_int(select.get(), 1);
        system.name            = db::column_get_string(select.get(), 2);
        system.planetsJson     = db::column_get_string(select.get(), 3);
        system.positionJson    = db::column_get_string(select.get(), 4);
        system.securityClass   = db::column_get_string(select.get(), 5);
        system.securityStatus  = sqlite3_column_double(select.get(), 6);
        system.starID          = sqlite3_column_int(select.get(), 7);
        system.stargatesJson   = db::column_get_string(select.get(), 8);
        system.stationsJson    = db::column_get_string(select.get(), 9);
        callback(system);
        return;

    } else if (results == 0) {
        HttpRequest request;
        request.hostname = "esi.evetech.net";
        request.target   = fmt::format("/v4/universe/systems/{0}/", solarSystemID);

        mIOState->makeAsyncHttpRequest(request, [this, solarSystemID, callback = std::move(callback)](auto &&response, auto &&) {
            SolarSystem system;
            const auto  j = json::parse(response.body);

            j.at("constellation_id").get_to(system.constellationID);
            j.at("name").get_to(system.name);
            system.planetsJson  = j.at("planets").dump();
            system.positionJson = j.at("position").dump();
            j.at("security_class").get_to(system.securityClass);
            j.at("security_status").get_to(system.securityStatus);
            j.at("star_id").get_to(system.starID);
            system.stargatesJson = j.at("stargates").dump();
            system.stationsJson  = j.at("stations").dump();
            j.at("system_id").get_to(system.systemID);

            assert(solarSystemID == system.systemID);
            callback(system);

            // Store the system in the database
            auto stmt = db::make_statement(mDbConnection, "INSERT INTO solarsystem VALUES(?,?,?,?,?,?,?,?,?,?)");
            sqlite3_bind_int(stmt.get(), 1, system.systemID);
            sqlite3_bind_int(stmt.get(), 2, system.constellationID);
            sqlite3_bind_text(stmt.get(), 3, system.name.c_str(), -1, nullptr);
            sqlite3_bind_text(stmt.get(), 4, system.planetsJson.c_str(), -1, nullptr);
            sqlite3_bind_text(stmt.get(), 5, system.positionJson.c_str(), -1, nullptr);
            sqlite3_bind_text(stmt.get(), 6, system.securityClass.c_str(), -1, nullptr);
            sqlite3_bind_double(stmt.get(), 7, system.securityStatus);
            sqlite3_bind_int(stmt.get(), 8, system.starID);
            sqlite3_bind_text(stmt.get(), 9, system.stargatesJson.c_str(), -1, nullptr);
            sqlite3_bind_text(stmt.get(), 10, system.stationsJson.c_str(), -1, nullptr);
            sqlite3_step(stmt.get());
        });
    } else {
        throw std::runtime_error(fmt::format("Found {0} solar systems with the id {1} in the database", results, solarSystemID));
    }
}

Killmail eo::EsiSession::resolveKillmail(int32 killmailid, const std::string &killmailhash)
{
    auto stmt = db::make_statement(mDbConnection, "SELECT COUNT(*) FROM killmail WHERE id = ? AND hash = ?;");
    sqlite3_bind_int(stmt.get(), 1, killmailid);
    sqlite3_bind_text(stmt.get(), 2, killmailhash.c_str(), -1, nullptr);
    sqlite3_step(stmt.get());
    if (const auto results = sqlite3_column_int(stmt.get(), 0); results == 1) {
        Killmail km;
        auto     select
            = db::make_statement(mDbConnection, "SELECT systemid, attackers, victim, killtime FROM killmail WHERE id = ? AND hash = ?");
        sqlite3_bind_int(select.get(), 1, killmailid);
        sqlite3_bind_text(select.get(), 2, killmailhash.c_str(), -1, nullptr);
        sqlite3_step(select.get());
        km.killmailID    = killmailid;
        km.killmailHash  = killmailhash;
        km.systemID      = sqlite3_column_int(select.get(), 0);
        km.attackersJson = db::column_get_string(select.get(), 1);
        km.victimJson    = db::column_get_string(select.get(), 2);
        km.killTime      = db::column_get_string(select.get(), 3);
        return km;
    } else if (results == 0) {
        Killmail    km;
        HttpRequest req;
        req.hostname = "esi.evetech.net";
        req.target   = fmt::format("/v1/killmails/{0}/{1}/", killmailid, killmailhash);

        const auto response = makeHttpRequest(req);
        const auto j        = json::parse(response.body);
        km.killmailID       = killmailid;
        km.killmailHash     = killmailhash;
        j.at("solar_system_id").get_to(km.systemID);
        km.attackersJson = j.at("attackers").dump();
        km.victimJson    = j.at("victim").dump();
        km.killTime      = j.at("killmail_time");
        stmt             = db::make_statement(mDbConnection, "INSERT INTO killmail VALUES(?,?,?,?,?,?)");
        sqlite3_bind_int(stmt.get(), 1, km.killmailID);
        sqlite3_bind_text(stmt.get(), 2, km.killmailHash.c_str(), -1, nullptr);
        sqlite3_bind_int(stmt.get(), 3, km.systemID);
        sqlite3_bind_text(stmt.get(), 4, km.attackersJson.c_str(), -1, nullptr);
        sqlite3_bind_text(stmt.get(), 5, km.victimJson.c_str(), -1, nullptr);
        sqlite3_bind_text(stmt.get(), 6, km.killTime.c_str(), -1, nullptr);
        sqlite3_step(stmt.get());
        return km;
    } else {
        throw std::runtime_error(fmt::format("Found {0} killmaiml with the id {1} in the database", results, killmailid));
    }
}

void eo::EsiSession::resolveKillmailAsync(int32 killmailid, const std::string &killmailhash, std::function<void(const Killmail &)> callback)
{
    auto stmt = db::make_statement(mDbConnection, "SELECT COUNT(*) FROM killmail WHERE id = ? AND hash = ?;");
    sqlite3_bind_int(stmt.get(), 1, killmailid);
    sqlite3_bind_text(stmt.get(), 2, killmailhash.c_str(), -1, nullptr);
    sqlite3_step(stmt.get());
    if (const auto results = sqlite3_column_int(stmt.get(), 0); results == 1) {
        Killmail km;
        auto     select
            = db::make_statement(mDbConnection, "SELECT systemid, attackers, victim, killtime FROM killmail WHERE id = ? AND hash = ?");
        sqlite3_bind_int(select.get(), 1, killmailid);
        sqlite3_bind_text(select.get(), 2, killmailhash.c_str(), -1, nullptr);
        sqlite3_step(select.get());
        km.killmailID    = killmailid;
        km.killmailHash  = killmailhash;
        km.systemID      = sqlite3_column_int(select.get(), 0);
        km.attackersJson = db::column_get_string(select.get(), 1);
        km.victimJson    = db::column_get_string(select.get(), 2);
        km.killTime      = db::column_get_string(select.get(), 3);
        callback(km);
    } else if (results == 0) {
        HttpRequest req;
        req.hostname = "esi.evetech.net";
        req.target   = fmt::format("/v1/killmails/{0}/{1}/", killmailid, killmailhash);

        mIOState->makeAsyncHttpRequest(
            req, [this, killmailhash = killmailhash, killmailid, callback = std::move(callback)](auto &&response, auto &&) {
                Killmail   km;
                const auto j    = json::parse(response.body);
                km.killmailID   = killmailid;
                km.killmailHash = killmailhash;
                j.at("solar_system_id").get_to(km.systemID);
                km.attackersJson = j.at("attackers").dump();
                km.victimJson    = j.at("victim").dump();
                km.killTime      = j.at("killmail_time");
                callback(km);
                auto stmt = db::make_statement(mDbConnection, "INSERT INTO killmail VALUES(?,?,?,?,?,?)");
                sqlite3_bind_int(stmt.get(), 1, km.killmailID);
                sqlite3_bind_text(stmt.get(), 2, km.killmailHash.c_str(), -1, nullptr);
                sqlite3_bind_int(stmt.get(), 3, km.systemID);
                sqlite3_bind_text(stmt.get(), 4, km.attackersJson.c_str(), -1, nullptr);
                sqlite3_bind_text(stmt.get(), 5, km.victimJson.c_str(), -1, nullptr);
                sqlite3_bind_text(stmt.get(), 6, km.killTime.c_str(), -1, nullptr);
                sqlite3_step(stmt.get());
            });
    } else {
        throw std::runtime_error(fmt::format("Found {0} killmaiml with the id {1} in the database", results, killmailid));
    }
}

std::vector<ZkbKill> eo::EsiSession::getKillsInSystem(int32 solarsystemid, int limit)
{
    HttpRequest req;
    req.hostname = "zkillboard.com";
    req.target   = fmt::format("/api/kills/solarSystemID/{0}/", solarsystemid);

    const auto response = makeHttpRequest(req);
    const auto j        = json::parse(response.body);

    std::vector<ZkbKill> kills;
    kills.reserve(limit);
    int c = 0;
    for (auto &&item : j) {
        if (++c == limit) {
            break;
        }

        ZkbKill k;
        item.at("killmail_id").get_to(k.killmailID);
        const auto &zkbdata = item.at("zkb");
        zkbdata.at("hash").get_to(k.killmailHash);
        zkbdata.at("fittedValue").get_to(k.fittedValue);
        zkbdata.at("totalValue").get_to(k.totalValue);
        zkbdata.at("points").get_to(k.points);
        zkbdata.at("npc").get_to(k.npc);
        zkbdata.at("solo").get_to(k.solo);
        zkbdata.at("awox").get_to(k.awox);

        kills.push_back(std::move(k));
    }

    return kills;
}

void eo::EsiSession::getKillsInSystemAsync(int32 solarsystemid, int limit, std::function<void(const std::vector<esi::ZkbKill> &)> callback)
{
    HttpRequest req;
    req.hostname = "zkillboard.com";
    req.target   = fmt::format("/api/kills/solarSystemID/{0}/", solarsystemid);

    mIOState->makeAsyncHttpRequest(req, [limit, callback = std::move(callback)](auto &&response, auto &&) {
        const auto j = json::parse(response.body);

        std::vector<ZkbKill> kills;
        kills.reserve(limit);
        int c = 0;
        for (auto &&item : j) {
            if (++c == limit) {
                break;
            }

            ZkbKill k;
            item.at("killmail_id").get_to(k.killmailID);
            const auto &zkbdata = item.at("zkb");
            zkbdata.at("hash").get_to(k.killmailHash);
            zkbdata.at("fittedValue").get_to(k.fittedValue);
            zkbdata.at("totalValue").get_to(k.totalValue);
            zkbdata.at("points").get_to(k.points);
            zkbdata.at("npc").get_to(k.npc);
            zkbdata.at("solo").get_to(k.solo);
            zkbdata.at("awox").get_to(k.awox);

            kills.push_back(std::move(k));
        }
        callback(kills);
    });
}

void eo::EsiSession::convertCharacterIDAsync(int32 characterID, std::function<void(const esi::Character &)> callback)
{
    HttpRequest req;
    req.hostname = "esi.evetech.net";
    req.target   = fmt::format("/v4/characters/{0}/", characterID);

    mIOState->makeAsyncHttpRequest(std::move(req), [characterID, callback = std::move(callback)](auto &&resp, auto &&) {
        const auto     j = json::parse(resp.body);
        esi::Character character;
        try {
            j.at("alliance_id").get_to(character.allianceID);
        } catch (const json::out_of_range &) {
            character.allianceID = 0;
        }
        j.at("corporation_id").get_to(character.corpID);
        j.at("name").get_to(character.name);
        j.at("birthday").get_to(character.birthday);
        j.at("security_status").get_to(character.secStatus);
        character.characterID = characterID;

        callback(character);
    });
}

std::string eo::EsiSession::getTypeName(int32 invtypeid)
{
    auto stmt = db::make_statement(mDbConnection, "SELECT COUNT(*) FROM invTypes WHERE typeID = ?;");
    sqlite3_bind_int(stmt.get(), 1, invtypeid);
    sqlite3_step(stmt.get());
    if (sqlite3_column_int(stmt.get(), 0) != 1) {
        return fmt::format("INVALID - {0}", invtypeid); // This function should be frontend only anyway
    }

    stmt = db::make_statement(mDbConnection, "SELECT typeName FROM invTypes WHERE typeid = ? LIMIT 1;");
    sqlite3_bind_int(stmt.get(), 1, invtypeid);
    sqlite3_step(stmt.get());
    return db::column_get_string(stmt.get(), 0);
}
