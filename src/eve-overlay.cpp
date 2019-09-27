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
#include "imguiwindow.h"
#include "logging.h"
#include "requests.h"
#include "systeminfowindow.h"
#include <iostream>

#include <nlohmann/json.hpp>
#include <openssl/sha.h>

using json = nlohmann::json;

int main()
{
    auto iostate = std::make_shared<eo::IOState>();
    auto conn    = eo::db::make_database_connection();
    auto session = std::make_shared<eo::EsiSession>(conn);

    eo::HttpRequest r;
    r.hostname = "www.google.de";
    r.port     = "443";
    r.target   = "/";
    iostate->makeAsyncHttpRequest(r, [](auto &&response, auto &&) { eo::log::info("{0}", response.body); });

    /* const auto location = session->getCharacterLocation(); */
    /* const auto system   = eo::resolveSolarSystem(location.solarSystemID, conn); */

    /* eo::log::info("System name {0}", system.name); */
    eo::SystemInfoWindow window(session);

    while (!window.shouldWindowClose()) {
        window.pollEvents();
        window.frame();
        iostate->pollIoC();
    }

    return 0;
}
