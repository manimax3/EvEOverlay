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

#include "systeminfowindow.h"
#include "imgui.h"
#include "logging.h"

#include <algorithm>
#include <nlohmann/json.hpp>
#include <utility>

using json = nlohmann::json;

namespace {
std::string simplertimestring(const std::string &isotime)
{
    tm tm;
    strptime(isotime.c_str(), "%FT%TZ", &tm);
    std::array<char, 16> output = { 0 };
    const auto           length = std::strftime(output.data(), output.size(), "%d/%m %R", &tm);
    return std::string(output.data(), length);
}
}

eo::SystemInfoWindow::SystemInfoWindow(std::shared_ptr<EsiSession> session)
    : ImguiWindow(256, 256, "System Info Window", 0, 0)
    , mEsiSession(std::move(std::move(session)))
{
    cachedKillmails.reserve(20);
    fetchNextSystem(true);
}

void eo::SystemInfoWindow::fetchNextSystem(bool now)
{
    if ((std::chrono::steady_clock::now() - lastCheck) > refresh_system || now) {
        mEsiSession->getCharacterLocationAsync([this](auto &&location) {
            mEsiSession->resolveSolarSystemAsync(location.solarSystemID, [this](auto &&location) {
                if (location.systemID == currentSystem.systemID) {
                    return;
                }

                currentSystem = location;
                cachedKillmails.clear();
                cachedKillmails.reserve(3);

                mEsiSession->getKillsInSystemAsync(location.systemID, 20, [&](auto &&killmails) {
                    for (auto &&km : killmails) {
                        mEsiSession->resolveKillmailAsync(km.killmailID, km.killmailHash, [&, km](auto &&killmail) {
                            const auto j = json::parse(killmail.victimJson);
                            try {
                                const int32 characterID = j.at("character_id");
                                const int32 shipTypeID  = j.at("ship_type_id");
                                mEsiSession->convertCharacterIDAsync(characterID, [&, shipTypeID, km, killmail](auto &&character) {
                                    cachedKillmails.emplace_back(character, mEsiSession->getTypeName(shipTypeID), km.killmailID,
                                                                 simplertimestring(killmail.killTime), killmail.killTime);
                                    // TODO im too lazy to do this more efficient
                                    std::sort(begin(cachedKillmails), end(cachedKillmails),
                                              [](auto &&a, auto &&b) { return std::get<4>(a) > std::get<4>(b); });
                                });
                            } catch (const json::out_of_range &e) {
                            }
                        });
                    }
                });
            });
        });

        lastCheck = std::chrono::steady_clock::now();
    }
}

void eo::SystemInfoWindow::renderImguiContents()
{
    fetchNextSystem();
    if (ImGui::CollapsingHeader(currentSystem.name.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Columns(2);
        ImGui::Text("Name");
        ImGui::NextColumn();
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "%s", currentSystem.name.c_str());
        ImGui::NextColumn();

        ImGui::Text("Security Class");
        ImGui::NextColumn();
        ImGui::TextColored(ImVec4(1, 0.3, 0.3, 1), "%s", currentSystem.securityClass.c_str());
        ImGui::NextColumn();

        ImGui::Text("Security Status");
        ImGui::NextColumn();
        ImGui::TextColored(ImVec4(1, 0.3, 0.3, 1), "%.1f", currentSystem.securityStatus);
        ImGui::NextColumn();
        ImGui::Columns(1);
        ImGui::Separator();

        if (ImGui::CollapsingHeader("Last killmails")) {
            ImGui::Columns(4);
            for (const auto &km : cachedKillmails) {
                ImGui::Text("%s", std::get<0>(km).name.c_str());
                ImGui::NextColumn();
                ImGui::Text("%s", std::get<1>(km).c_str());
                ImGui::NextColumn();
                ImGui::PushID(std::get<2>(km));
                if (ImGui::Button("Open")) {
                    open_url_browser(fmt::format("https://zkillboard.com/kill/{0}/", std::get<2>(km)));
                }
                ImGui::PopID();
                ImGui::NextColumn();

                ImGui::Text("%s", std::get<3>(km).c_str());
                ImGui::NextColumn();
            }
            ImGui::Columns(1);
        }
    }
}
