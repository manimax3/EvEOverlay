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

eo::SystemInfoWindow::SystemInfoWindow(const std::shared_ptr<EsiSession> &session)
    : ImguiWindow(256, 256, "SystemInfoWindow", 0, 0)
    , mEsiSession(session)
{
    const auto character_location = mEsiSession->getCharacterLocation();
    currentSystem                 = resolveSolarSystem(character_location.solarSystemID, mEsiSession->getDbConnection());
}

void eo::SystemInfoWindow::fetchNextSystem()
{
    if ((std::chrono::steady_clock::now() - lastCheck) > refresh_system) {
        const auto character_location = mEsiSession->getCharacterLocation();
        if (character_location.solarSystemID != currentSystem.systemID) {
            currentSystem = resolveSolarSystem(character_location.solarSystemID, mEsiSession->getDbConnection());
        }

		lastCheck = std::chrono::steady_clock::now();
    }
}

void eo::SystemInfoWindow::renderImguiContents()
{
    fetchNextSystem();
    if (ImGui::CollapsingHeader(currentSystem.name.c_str())) {
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
    }
}
