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

#include "requests.h"
#include <iomanip>
#include <iostream>
#include <sstream>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>

namespace eo {
namespace ssl = boost::asio::ssl;
using tcp     = net::ip::tcp;

class AsyncHttpRequest : public std::enable_shared_from_this<AsyncHttpRequest> {
public:
    explicit AsyncHttpRequest(HttpRequest r, IOState &state, std::function<void(const HttpResponse &, IOState &)> callback)
        : request(std::move(r))
        , ctx(ssl::context::tlsv12_client)
        , mStream(*state.getIoC(), ctx)
        , mResolver(*state.getIoC())
        , mCallback(std::move(callback))
        , mIOState(state)
    {
    }

    void run()
    {
        ctx.set_default_verify_paths();
        mResolver.async_resolve(request.hostname, request.port,
                                beast::bind_front_handler(&AsyncHttpRequest::on_resolve, shared_from_this()));
    }

    void on_resolve(beast::error_code ec, tcp::resolver::results_type results)
    {
        beast::get_lowest_layer(mStream).expires_after(std::chrono::seconds(30));
        beast::get_lowest_layer(mStream).async_connect(results,
                                                       beast::bind_front_handler(&AsyncHttpRequest::on_connect, shared_from_this()));
    }

    void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type)
    {
        mStream.async_handshake(ssl::stream_base::client, beast::bind_front_handler(&AsyncHttpRequest::on_handshake, shared_from_this()));
    }

    void on_handshake(beast::error_code ec)
    {
        beast::get_lowest_layer(mStream).expires_after(std::chrono::seconds(30));

        httprequest = { request.requestType == eo::HttpRequest::GET ? http::verb::get : http::verb::post, request.target, 11 };

        const auto makevisitor = [&httprequest = httprequest](const auto &value) {
            return [&value, &httprequest](const auto &header) { httprequest.set(header, value); };
        };

        httprequest.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        httprequest.set(http::field::host, request.hostname);

        for (auto &[field, value] : request.headers) {
            const auto visitor = makevisitor(value);
            std::visit(visitor, field);
        }

        httprequest.body() = request.body;
        httprequest.prepare_payload();

        http::async_write(mStream, httprequest, beast::bind_front_handler(&AsyncHttpRequest::on_write, shared_from_this()));
    }

    void on_write(beast::error_code ec, std::size_t bytes_transferred)
    {
        http::async_read(mStream, buffer, httpresponse, beast::bind_front_handler(&AsyncHttpRequest::on_read, shared_from_this()));
    }

    void on_read(beast::error_code ec, std::size_t bytes_transferred)
    {
        response.statusCode = 200;
        response.body       = std::move(httpresponse.body());

        for (const auto &field : httpresponse) {
            FieldMap::key_type    key{ field.name() };
            FieldMap::mapped_type value{ field.value() };
            response.headers[key] = std::move(value);
        }

        net::post(*mIOState.getIoC(), bind(mCallback, std::move(response), mIOState));
        mStream.async_shutdown([kp = shared_from_this()](auto &&) {});
    }

private:
    std::function<void(const HttpResponse &, IOState &)> mCallback;

    IOState &                            mIOState;
    HttpRequest                          request;
    HttpResponse                         response;
    ssl::context                         ctx;
    beast::ssl_stream<beast::tcp_stream> mStream;
    tcp::resolver                        mResolver;
    http::request<http::empty_body>      req;
    beast::flat_buffer                   buffer;
    http::response<http::string_body>    httpresponse;
    http::request<http::string_body>     httprequest;
};
}

eo::IOState::IOState() { mIoContext = std::make_shared<net::io_context>(); }

void eo::IOState::pollIoC() { mIoContext->poll(); }

void eo::IOState::runIoC() { mIoContext->run(); }

void eo::IOState::makeAsyncHttpRequest(const struct HttpRequest &                                  request,
                                       std::function<void(const struct HttpResponse &, IOState &)> callback)
{
    auto asyncrequest = std::make_shared<AsyncHttpRequest>(request, *this, std::move(callback));
    asyncrequest->run();
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
    net::io_context ioc;
    ssl::context    ctx(ssl::context::tlsv12_client);
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
    const auto      address = net::ip::make_address(ip);
    net::io_context ioc;
    tcp::acceptor   acceptor{ ioc, { address, port } };
    tcp::socket     socket{ ioc };
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
