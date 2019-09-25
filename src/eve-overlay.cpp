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

#include "authentication.h"
#include "base64.h"
#include "db.h"
#include "esisession.h"
#include "gfx/imguiwindow.h"
#include "logging.h"
#include "requests.h"
#include <iostream>

#include <nlohmann/json.hpp>
#include <openssl/sha.h>

using json = nlohmann::json;

int main()
{
    /* eo::ImguiWindow window(200, 200, "Test", 0, 0); */
    /* eo::ImguiWindow window2(200, 200, "Test", 0, 0); */

    /* eo::open_url_browser("www.google.de"); */
    /* eo::test_get(); */

    /* std::array<unsigned char, SHA256_DIGEST_LENGTH> output; */
    /* const char* msg = "Hall"; */
    /* SHA256((unsigned char*) msg, 4, (unsigned char*) output.data()); */
    /* std::cout << base64_decode(base64_encode(output.data(), output.size())) << std::endl; */

    /* std::cout << eo::urlencode("Hallo?#Was") << std::endl; */

    /* while(!window.shouldWindowClose() && !window2.shouldWindowClose()) { */
    /* 	window.pollEvents(); */
    /* 	window.frame(); */
    /* 	window2.frame(); */
    /* } */

    /* const auto code_challenge = eo::make_authorize_request({ "esi-characters.read_blueprints.v1" }); */
    /* const auto auth_code      = eo::handle_redirect(); */
    /* const auto tokenrequest   = eo::make_token_request(auth_code, code_challenge); */
    /* const auto verifyresult   = eo::verify_token(tokenrequest.access_token); */
    /* eo::log::info(verifyresult.characterName); */

    auto           conn = eo::db::make_database_connection();
    eo::EsiSession session{ conn };

    const auto location = session.getCharacterLocation();
    const auto system   = eo::resolveSolarSystem(location.solarSystemID, conn);

    eo::log::info("System name {0}", system.name);

    /* eo::db::store_in_db(conn, eo::make_token_data(tokenrequest, verifyresult)); */

    /* try { */
    /*     auto tokendata = eo::db::get_latest_tokendata_by_expiredate(conn); */
    /*     eo::log::info("TokenData Dump:\n{0}", json(tokendata).dump(2)); */
    /*     eo::log::info("Token expired? {0}", eo::token_expired(tokendata)); */
    /* } catch (const std::runtime_error &e) { */
    /*     eo::log::error("Couldnt find a token in the databse"); */
    /* } */

    // We need to store this somewhere
    // Verify request also get the expiration date
    // store the character
    //
    // a way to retrieve the token
    // refresh the token if needed

    return 0;
}
