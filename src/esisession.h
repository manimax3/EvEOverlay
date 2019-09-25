#pragma once
#include "authentication.h"
#include "db.h"

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
}

/*
 * Handle esi request which require authentication
 */
class EsiSession {
public:
    // Loads the token or make the authentication routine
    // TODO No mutly character support here
    explicit EsiSession(db::SqliteSPtr dbconnection = db::make_database_connection());

    esi::CharacterLocation getCharacterLocation();

private:
    // Make sure this is alwasys valid
	// Invariant: Valid token which might be expired
    TokenData mCurrentToken;

    // We keep a connection alive
    db::SqliteSPtr mDbConnection;
};

/*
 * Resolve a solarsystemid.
 * if dbconnection is privided trys to load the solarsystem from there and/or stores it there after make the
 * esi resolve request
 */
esi::SolarSystem resolveSolarSystem(int32 solarSystemID, db::SqliteSPtr dbconnection = {});

}
