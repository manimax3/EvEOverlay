#include "authentication.h"
#include "base64.h"
#include "logging.h"
#include "requests.h"

#include <array>
#include <random>
#include <sstream>
#include <string>

#include <openssl/sha.h>

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
    out << "https://" << eve_baseurl << "v2/oauth/authorize/?response_type=code"
        << "&redirect_uri=" << urlencode(std::string(redirect_url)) << "&client_id=" << client_id;

    const auto space = urlencode(" ");

    std::string last{};
    bool        foundone = false;
    if (scopes.size() > 0) {
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

    out << "&state=somethingunique";

	const auto url = out.str();
    log::info(url);
	open_url_browser(url);


    return code_challenge;
}
