#include "requests.h"
#include <iomanip>
#include <sstream>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>

namespace eo {
namespace ssl = boost::asio::ssl;
using tcp     = net::ip::tcp;
}
namespace {
auto &get_io_context()
{
    static eo::net::io_context context;
    return context;
}
}

void eo::open_url_browser(const std::string &url)
{
#ifdef __linux__
    system(("xdg-open '" + url + "'").c_str());
#else
    static_assert(false, "This OS is currently not supported");
#endif
}

std::string eo::urlencode(const std::string &input)
{
    // TODO ostringstream is propably slower than just a string
    std::ostringstream result;
    result.fill('0');

    for (char c : input) {

        if (std::isalnum(c) || c == '-' || c == '.' || c == '_' || c == '~') {
            result << c;
        } else {
            result << '%' << std::hex << std::setw(2) << static_cast<int>(c);
        }
    }

    return result.str();
}

std::string eo::base64_safe(const std::string &base64)
{
    // TODO ostringstream is propably slower than just a string
    std::ostringstream out;

    for (char c : base64) {
        if (c == '+') {
            out << '-';
        } else if (c == '/') {
            out << '_';
        } else if (c == '=') {
            continue;
        } else {
            out << c;
        }
    }

    return out.str();
}

eo::HttpResponse eo::makeHttpRequest(const HttpRequest &request)
{
    auto &       ioc = get_io_context();
    ssl::context ctx(ssl::context::tlsv12_client);
    ctx.set_default_verify_paths();

    tcp::resolver                        resolver(ioc);
    beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);
    const auto                           results = resolver.resolve(request.hostname, request.port);

    beast::get_lowest_layer(stream).connect(results);
    stream.handshake(ssl::stream_base::client);
    {
        http::request<http::string_body> httprequest{ request.requestType == eo::HttpRequest::GET ? http::verb::get : http::verb::post,
                                                      request.target, 11 };

        const auto makevisitor
            = [&httprequest](const auto &value) { return [&value, &httprequest](const auto &header) { httprequest.set(header, value); }; };

        httprequest.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        httprequest.set(http::field::host, request.hostname);

        for (auto &[field, value] : request.headers) {
            const auto visitor = makevisitor(value);
            std::visit(visitor, field);
        }

        httprequest.body() = request.body;
        httprequest.prepare_payload();
        http::write(stream, httprequest);
    }

    beast::flat_buffer                buffer;
    http::response<http::string_body> httpresponse;
    http::read(stream, buffer, httpresponse);

    HttpResponse response;
    response.statusCode = 200;
    response.body       = std::move(httpresponse.body());

    for (const auto &field : httpresponse) {
        FieldMap::key_type    key{ field.name() };
        FieldMap::mapped_type value{ field.value() };
        response.headers[key] = std::move(value);
    }

    return response;
}

eo::HttpRequest eo::expectHttpRequest(const HttpResponse &response, unsigned short port, const std::string &ip)
{
    const auto    address = net::ip::make_address(ip);
    auto &        ioc     = get_io_context();
    tcp::acceptor acceptor{ ioc, { address, port } };
    tcp::socket   socket{ ioc };
    acceptor.accept(socket);

    beast::flat_buffer               buffer;
    http::request<http::string_body> httprequest;
    http::read(socket, buffer, httprequest);

    http::response<http::string_body> httpresponse{ std::piecewise_construct };
    httpresponse.body() = response.body;

    const auto makevisitor
        = [&httpresponse](const auto &value) { return [&value, &httpresponse](const auto &header) { httpresponse.set(header, value); }; };

    httpresponse.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

    for (auto &[field, value] : response.headers) {
        const auto visitor = makevisitor(value);
        std::visit(visitor, field);
    }

    http::write(socket, httpresponse);

    HttpRequest request;
    request.target = std::string(httprequest.target());
    request.body   = httprequest.body();
    for (const auto &field : httprequest) {
        FieldMap::key_type    key{ field.name() };
        FieldMap::mapped_type value{ field.value() };
        request.headers[key] = std::move(value);
    }
    // TODO Type, port, hostname
    return request;
}

