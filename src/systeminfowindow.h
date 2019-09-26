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
#include "esisession.h"
#include "imguiwindow.h"

#include <chrono>

namespace eo {
class SystemInfoWindow : public ImguiWindow {
public:
    constexpr static auto refresh_system = std::chrono::seconds(30);
    explicit SystemInfoWindow(const std::shared_ptr<EsiSession> &esisession);

protected:
    void renderImguiContents() override;

	void fetchNextSystem();

private:
    esi::SolarSystem                      currentSystem{};
    std::shared_ptr<EsiSession>           mEsiSession{};
    std::chrono::steady_clock::time_point lastCheck = std::chrono::steady_clock::now();
};
}
