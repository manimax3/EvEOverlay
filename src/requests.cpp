#include "requests.h"
#include <iomanip>
#include <iostream>
#include <sstream>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/predef/os/linux.h>

void eo::open_url_browser(const std::string &url)
{
#if BOOST_OS_LINUX
    system(("xdg-open '" + url + "'").c_str());
    std::cout << std::endl; // Just because system() is so weird
#else
	static_assert(false, "This OS is currently not supported");
#endif
}

std::string eo::urlencode(const std::string &input)
{
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
