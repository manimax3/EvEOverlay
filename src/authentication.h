#pragma once
#include "util.h"

#include <list>
#include <string_view>

namespace eo {
constexpr std::string_view client_id    = "fd612fe6fd514fd5b4c1718ed72ef33e";
constexpr std::string_view redirect_url = "http://localhost/callback/";
constexpr std::string_view eve_baseurl  = "login.eveonline.com";

std::string make_authorize_request(std::list<std::string> scopes);
std::string handle_redirect();
}
