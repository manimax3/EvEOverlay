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

#include "authentication.h"
#include "base64.h"
#include "db.h"
#include "esisession.h"
#include "gfx/imguiwindow.h"
#include "logging.h"
#include "requests.h"
#include <iostream>

#include <nlohmann/json.hpp>
#include <openssl/sha.h>

using json = nlohmann::json;

int main()
{
    auto           conn = eo::db::make_database_connection();
    eo::EsiSession session{ conn };

    const auto location = session.getCharacterLocation();
    const auto system   = eo::resolveSolarSystem(location.solarSystemID, conn);

    eo::log::info("System name {0}", system.name);
    eo::ImguiWindow window(200, 200, "Test", 0, 0);

    while (!window.shouldWindowClose()) {
        window.pollEvents();
        window.frame();
    }

    return 0;
}
