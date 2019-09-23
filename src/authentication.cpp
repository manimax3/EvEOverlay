#include "authentication.h"
#include "base64.h"
#include "logging.h"
#include "requests.h"

#include <array>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <string>

#include <nlohmann/json.hpp>
#include <openssl/sha.h>

using json = nlohmann::json;

namespace {
std::string random_string(int n)
{
    constexpr std::string_view                             chars = "0123456789abcdefghijklmnopqrstuvwxzyABCDEFGHIJKLMNOPQRSTUVWXZY";
    thread_local static std::default_random_engine         rng{ std::random_device{}() };
    thread_local static std::uniform_int_distribution<int> dist(0, chars.length() - 1);

    std::string out;
    out.reserve(n);

    for (int i = 0; i < n; i++) {
        out += chars[dist(rng)];
    }

    return out;
}
}

std::string eo::make_authorize_request(std::list<std::string> scopes)
{

    std::ostringstream out;
    out << "https://" << eve_baseurl << "/v2/oauth/authorize/?response_type=code"
        << "&redirect_uri=" << urlencode(std::string(redirect_url)) << "&client_id=" << client_id;

    const auto space = urlencode(" ");

    std::string last{};
    bool        foundone = false;
    if (scopes.size() > 0) {
        out << "&scopes=";
        last = scopes.back();
        scopes.pop_back();
        foundone = true;
    }

    for (const auto &scope : scopes) {
        out << urlencode(scope);
        out << space;
    }

    if (foundone) {
        out << last;
    }

    const auto code_challenge = base64_safe(base64_encode((byte *)random_string(32).c_str(), 32));

    std::array<byte, 32> sha_output;
    SHA256((byte *)code_challenge.data(), code_challenge.length(), sha_output.data());

    out << "&code_challenge=" << base64_safe(base64_encode(sha_output.data(), 32));
    out << "&code_challenge_method=S256";

    out << "&state=somethingunique";

    const auto url = out.str();
    log::info("Making authorization request to: {0}", url);
    open_url_browser(url);

    return code_challenge;
}

std::string eo::handle_redirect()
{
    HttpResponse response;
    response.body                               = "<html><body>You can close this now.</body></html>";
    response.headers[http::field::content_type] = "text/html";

    const auto request = expectHttpRequest(response);

    const auto &   target   = std::string{ request.target };
    constexpr auto code_str = std::string_view("code=");

    const auto code_pos = target.find(code_str);
    if (code_pos == target.npos) {
        // TODO error
        log::error("Redirect did not contain a code parameter");
        return "error";
    }

    std::string code;
    for (int i = 0; code_pos + code_str.length() - 1 + i != target.length() - 1; i++) {
        if (target[code_pos + code_str.length() + i] == '&') {
            break;
        }

        code += target[code_pos + code_str.length() + i];
    }

    return code;
}

eo::TokenRequestResult eo::make_token_request(const AuthenticationCode &auth_code, const CodeChallenge &code_challenge)
{
    eo::HttpRequest request;
    request.hostname                           = eve_baseurl;
    request.port                               = "443";
    request.requestType                        = HttpRequest::POST;
    request.target                             = "/v2/oauth/token";
    request.headers[http::field::content_type] = "application/x-www-form-urlencoded";
    request.body = fmt::format("grant_type=authorization_code&code={0}&client_id={1}&code_verifier={2}", auth_code, std::string(client_id),
                               code_challenge);

    const auto response = makeHttpRequest(request);

    log::info(response.body);
    auto j = json::parse(response.body);

    return TokenRequestResult{ j.at("access_token"), j.at("expires_in"), j.at("token_type"), j.at("refresh_token") };
}

eo::VerifyTokenRequestResult eo::verify_token(const AuthenticationCode &auth_code)
{
    HttpRequest request;
    request.hostname                            = "esi.evetech.net";
    request.target                              = "/verify/";
    request.headers["X-User-Agent"]             = client_id;
    request.headers[http::field::authorization] = fmt::format("Bearer {0}", auth_code);
    request.headers[http::field::accept]        = "application/json";

    const auto response = makeHttpRequest(request);

    json j = json::parse(response.body);
    return { j["CharacterID"], j["CharacterName"], j["CharacterOwnerHash"], j["ExpiresOn"], j["TokenType"] };
}

eo::TokenData eo::load_token_data()
{
    json          j;
    std::ifstream ifs(settings_file_path);
    ifs >> j;
    return j.get<eo::TokenData>();
}

void eo::save_token_data(const eo::TokenData &data)
{
    std::ofstream ofs(settings_file_path);
    json          j = data;
    ofs << j;
}
