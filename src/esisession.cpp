#include "esisession.h"
#include "logging.h"
#include "requests.h"

#include <fmt/core.h>
#include <nlohmann/json.hpp>
#include <sqlite3.h>

using namespace eo::esi;
using json = nlohmann::json;

eo::EsiSession::EsiSession(db::SqliteSPtr dbconnection)
    : mDbConnection(std::move(dbconnection))
{
    if (!mDbConnection) {
        throw std::logic_error("EsiSession requries a valid dbconnection");
    }

    TokenData token;
    // TODO Quite the common case, should not be handled by exception
    try {
        token = db::get_latest_tokendata_by_expiredate(mDbConnection);
        if (token_expired(token)) {
            refresh_token(token);
        }
    } catch (const std::runtime_error &re) {
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
    log::info("{0}", response.body);
    const auto j = json::parse(response.body);

    CharacterLocation location;
    j.at("solar_system_id").get_to(location.solarSystemID);

    // TODO Quite the common case, should not be handled by exception
    try {
        j.at("station_id").get_to(location.stationID);
        j.at("structure_id").get_to(location.structureID);
    } catch (const json::out_of_range &e) {
        // Did not contain any staion or structure information
    }

    return location;
}

SolarSystem eo::resolveSolarSystem(int32 solarSystemID, db::SqliteSPtr dbconnection)
{
    SolarSystem system;
    if (!dbconnection) {
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
        auto stmt = db::make_statement(dbconnection, "SELECT COUNT(*) FROM solarsystem WHERE id = ?;");
        sqlite3_bind_int(stmt.get(), 1, solarSystemID);
        sqlite3_step(stmt.get());
        if (const auto results = sqlite3_column_int(stmt.get(), 0); results == 1) {
            auto select = db::make_statement(dbconnection, "SELECT * FROM solarsystem WHERE id = ? LIMIT 1;");
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
            system = eo::resolveSolarSystem(solarSystemID, nullptr);
        } else {
            throw std::runtime_error(fmt::format("Found {0} solar systems with the id {1} in the database", results, solarSystemID));
        }

        // Store the system in the database
        stmt = db::make_statement(std::move(dbconnection), "INSERT INTO solarsystem VALUES(?,?,?,?,?,?,?,?,?,?)");
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
    }

    return system;
}
