#pragma once
#include "util.h"

#include <list>
#include <string>
#include <string_view>

namespace eo {
constexpr std::string_view client_id    = "fd612fe6fd514fd5b4c1718ed72ef33e";
constexpr std::string_view redirect_url = "http://localhost:8080/callback/";
constexpr std::string_view eve_baseurl  = "login.eveonline.com";

struct TokenRequestResult {
    std::string access_token;
    int         expires_in;
    std::string token_type;
    std::string refresh_token;
    std::string codeChallenge;
};

struct VerifyTokenRequestResult {
    int32       characterID;
    std::string characterName;
    std::string characterOwnerHash;
    std::string expiresOn;
    std::string tokenType;
};

struct TokenData {
    std::string refreshToken;
    std::string characterName;
    int32       characterID;
    std::string accessToken;
    std::string expiresOn;
    std::string codeChallenge;
};

using AuthenticationCode = std::string;
using CodeChallenge      = std::string;

CodeChallenge            make_authorize_request(std::list<std::string> scopes);
AuthenticationCode       handle_redirect();
TokenRequestResult       make_token_request(const AuthenticationCode &auth_code, const CodeChallenge &code_challenge);
VerifyTokenRequestResult verify_token(const AuthenticationCode &auth_code);
void                     refresh_token(TokenData &token);
bool                     token_expired(const TokenData &token);

inline TokenData make_token_data(const TokenRequestResult &rresult, const VerifyTokenRequestResult &vresult)
{
    return { rresult.refresh_token, vresult.characterName, vresult.characterID,
             rresult.access_token,  vresult.expiresOn,     rresult.codeChallenge };
}
}
