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
#include "util.h"
#include <map>
#include <memory>
#include <variant>

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/beast/http/field.hpp>

namespace boost::asio {
struct io_context;
}

namespace eo {

// Namespace shortcuts
namespace beast = boost::beast;
namespace http  = beast::http;
namespace net   = boost::asio;
using FieldMap  = std::map<std::variant<http::field, std::string>, std::string>;

class IOState {
public:
    IOState();

    inline auto &getIoC() { return mIoContext; }

    void pollIoC();
    void runIoC();

    void makeAsyncHttpRequest(const struct HttpRequest &request, std::function<void(const struct HttpResponse &, IOState &)> callback);

private:
    std::shared_ptr<net::io_context> mIoContext;
    net::executor_work_guard<net::io_context::executor_type> workGuard;
};

void        open_url_browser(const std::string &url);
std::string urlencode(const std::string &input);
std::string base64_safe(const std::string &base64);

struct HttpResponse {
    int         statusCode;
    FieldMap    headers;
    std::string body;
};

struct HttpRequest {
    enum Type { GET, POST };
    std::string hostname;
    Type        requestType = GET;
    std::string target = "/";
    FieldMap    headers{};
    std::string body = {};
    std::string port = "443";
};

HttpResponse makeHttpRequest(const HttpRequest &request);
HttpRequest  expectHttpRequest(const HttpResponse &response, unsigned short port = 8080, const std::string &ip = "0.0.0.0");
}
