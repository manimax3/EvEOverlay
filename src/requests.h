#pragma once
#include <map>

namespace eo {
void        open_url_browser(const std::string &url);
void        test_get();
std::string urlencode(const std::string &input);
std::string base64_safe(const std::string &base64);
}
