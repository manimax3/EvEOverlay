#include "authentication.h"
#include "base64.h"
#include "logging.h"
#include "requests.h"

#include <array>
#include <iostream>
#include <random>
#include <sstream>
#include <string>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>

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
    namespace beast = boost::beast;
    namespace http  = beast::http;
    namespace net   = boost::asio;
    namespace ssl   = net::ssl;
    using tcp       = net::ip::tcp;

    const auto address = net::ip::make_address("0.0.0.0");

    net::io_context ioc;
    tcp::acceptor   acceptor{ ioc, { address, 8080 } };
    tcp::socket     socket{ ioc };
    acceptor.accept(socket);

    beast::flat_buffer               buffer;
    http::request<http::string_body> req;
    http::read(socket, buffer, req);

    http::response<http::string_body> resp{ std::piecewise_construct };
    resp.body() = "<html><body>You can close this now.</body></html>";
    resp.set(http::field::content_type, "text/html");
    resp.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    http::write(socket, resp);

    const auto &   target   = std::string{ req.target() };
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

std::tuple<std::string, int, std::string, std::string> eo::make_token_request(const std::string &auth_code,
                                                                              const std::string &code_challenge)
{
    namespace beast = boost::beast;
    namespace http  = beast::http;
    namespace net   = boost::asio;
    namespace ssl   = net::ssl;
    using tcp       = net::ip::tcp;

    net::io_context ioc;

    ssl::context ctx(ssl::context::tlsv12_client);
    ctx.set_default_verify_paths();

    tcp::resolver                        resolver(ioc);
    beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);

    const auto results = resolver.resolve(eve_baseurl, "443");
    beast::get_lowest_layer(stream).connect(results);

    stream.handshake(ssl::stream_base::client);

    http::request<http::string_body> req{ http::verb::post, "/v2/oauth/token", 11 };
    req.set(http::field::host, eve_baseurl);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    req.set(http::field::content_type, "application/x-www-form-urlencoded");

    const auto body = fmt::format("grant_type=authorization_code&code={0}&client_id={1}&code_verifier={2}", auth_code,
                                  std::string(client_id), code_challenge);

    req.content_length(body.length());
    req.body() = body;

    http::write(stream, req);
    beast::flat_buffer buffer;

    http::response<http::dynamic_body> res;
    http::read(stream, buffer, res);

    std::cout << res << '\n';

    return {};
}
