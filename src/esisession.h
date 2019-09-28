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
#include "authentication.h"
#include "db.h"
#include "requests.h"

#include <vector>

namespace eo {

/*
 * Esi datastructures for easier management
 */
namespace esi {
    struct CharacterLocation {
        int32 solarSystemID;
        int32 stationID;
        int32 structureID;
    };

    struct SolarSystem {
        int32       systemID;
        int32       constellationID;
        std::string name;
        std::string planetsJson;
        std::string positionJson;
        std::string securityClass;
        float       securityStatus;
        int32       starID;
        std::string stargatesJson;
        std::string stationsJson;
    };

    struct Killmail {
        int32       killmailID;
        std::string killmailHash;
        int32       systemID;
        std::string attackersJson;
        std::string victimJson;
    };

    struct ZkbKill {
        int32       killmailID;
        std::string killmailHash;
        double      fittedValue;
        double      totalValue;
        int32       points;
        bool        npc;
        bool        solo;
        bool        awox;
    };
}

/*
 * Handle esi request which require authentication
 */
class EsiSession {
public:
    // Loads the token or make the authentication routine
    // TODO No mutly character support here
    explicit EsiSession(db::SqliteSPtr dbconnection, std::shared_ptr<IOState> iostate);

    // Looks up in the database if no entry then does and http request
    esi::CharacterLocation getCharacterLocation();

    void getCharacterLocationAsync(std::function<void(const esi::CharacterLocation &)> callback);

    esi::SolarSystem resolveSolarSystem(int32 soalarSystemID);

    void resolveSolarSystemAsync(int32 soalarSystemID, std::function<void(const esi::SolarSystem &)> callback);

    esi::Killmail resolveKillmail(int32 killmailid, const std::string &killmailhash);

    void resolveKillmailAsync(int32 killmailid, const std::string &killmailhash, std::function<void(const esi::Killmail &)> callback);

    std::string getTypeName(int32 invtypeid);

    db::SqliteSPtr getDbConnection() const { return mDbConnection; }
    IOState &      getIOState() { return *mIOState; }

private:
    // Make sure this is alwasys valid
    // Invariant: Valid token which might be expired
    TokenData mCurrentToken;

    // We keep a connection alive
    db::SqliteSPtr mDbConnection;

    std::shared_ptr<IOState> mIOState;
};
}
