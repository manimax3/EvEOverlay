#include "authentication.h"
#include "base64.h"
#include "db.h"
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

    /* eo::save_token_data(eo::make_token_data(tokenrequest, verifyresult)); */

    auto conn = eo::db::make_database_connection();
    /* eo::db::store_in_db(conn, eo::make_token_data(tokenrequest, verifyresult)); */

    try {
        auto tokendata = eo::db::get_latest_tokendata_by_expiredate(conn);
        std::cout << json(tokendata) << std::endl;
    } catch (const std::runtime_error &e) {
		eo::log::error("Couldnt find a token in the databse");
    }

    // We need to store this somewhere
    // Verify request also get the expiration date
    // store the character
    //
    // a way to retrieve the token
    // refresh the token if needed

    return 0;
}
