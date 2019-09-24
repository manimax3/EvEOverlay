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
};

using AuthenticationCode = std::string;
using CodeChallenge      = std::string;

CodeChallenge            make_authorize_request(std::list<std::string> scopes);
AuthenticationCode       handle_redirect();
TokenRequestResult       make_token_request(const AuthenticationCode &auth_code, const CodeChallenge &code_challenge);
VerifyTokenRequestResult verify_token(const AuthenticationCode &auth_code);
void                     refresh_token(TokenData &token);

inline TokenData make_token_data(const TokenRequestResult &rresult, const VerifyTokenRequestResult &vresult)
{
    return { rresult.refresh_token, vresult.characterName, vresult.characterID, rresult.access_token, vresult.expiresOn };
}


// Serialization

template<typename Json>
void to_json(Json &json, const TokenData &data)
{
    json = Json{ { "refreshToken", data.refreshToken },
                 { "characterName", data.characterName },
                 { "characterID", data.characterID },
                 { "accessToken", data.accessToken },
                 { "expiresOn", data.expiresOn } };
}

template<typename Json>
void from_json(const Json &json, TokenData &data)
{
    json.at("refreshToken").get_to(data.refreshToken);
    json.at("characterName").get_to(data.characterName);
    json.at("characterID").get_to(data.characterID);
    json.at("accessToken").get_to(data.accessToken);
    json.at("expiresOn").get_to(data.expiresOn);
}

template<typename Json>
void to_json(Json &json, const TokenRequestResult &result)
{
    json = Json{ { "access_token", result.access_token },
                 { "expires_in", result.expires_in },
                 { "token_type", result.token_type },
                 { "refresh_token", result.refresh_token } };
}

template<typename Json>
void from_json(const Json &json, TokenRequestResult &result)
{
    json.at("access_token").get_to(result.access_token);
    json.at("expires_in").get_to(result.expires_in);
    json.at("token_type").get_to(result.token_type);
    json.at("refresh_token").get_to(result.refresh_token);
}

template<typename Json>
void to_json(Json &json, const VerifyTokenRequestResult &result)
{
    json = Json{ { "characterID", result.characterID },
                 { "characterName", result.characterName },
                 { "characterOwnerHash", result.characterOwnerHash },
                 { "expiresOn", result.expiresOn },
                 { "token_type", result.tokenType } };
}

template<typename Json>
void from_json(const Json &json, VerifyTokenRequestResult &result)
{
    json.at("characterID").get_to(result.characterID);
    json.at("characterName").get_to(result.characterName);
    json.at("characterOwnerHash").get_to(result.characterOwnerHash);
    json.at("expiresOn").get_to(result.expiresOn);
    json.at("token_type").get_to(result.tokenType);
}
}
