#pragma once
#include "util.h"

#include <list>
#include <string_view>

namespace eo {
constexpr std::string_view client_id    = "fd612fe6fd514fd5b4c1718ed72ef33e";
constexpr std::string_view redirect_url = "http://localhost:8080/callback/";
constexpr std::string_view eve_baseurl  = "login.eveonline.com";

std::string                                            make_authorize_request(std::list<std::string> scopes);
std::string                                            handle_redirect();
std::tuple<std::string, int, std::string, std::string> make_token_request(const std::string &auth_code, const std::string &code_challenge);
}
