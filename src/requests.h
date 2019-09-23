#pragma once
#include "util.h"
#include <map>
#include <variant>

#include <boost/beast/http/field.hpp>

namespace eo {

// Namespace shortcuts
namespace beast = boost::beast;
namespace http  = beast::http;
namespace net   = boost::asio;
using FieldMap  = std::map<std::variant<http::field, std::string>, std::string>;

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
    std::string target;
    FieldMap    headers;
    std::string body = {};
    std::string port = "443";
};

HttpResponse makeHttpRequest(const HttpRequest &request);
HttpRequest  expectHttpRequest(const HttpResponse &response, unsigned short port = 8080, const std::string &ip = "0.0.0.0");
}
