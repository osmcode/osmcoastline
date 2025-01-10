#ifndef OSMCOASTLINE_HPP
#define OSMCOASTLINE_HPP

/*

  Copyright 2012-2025 Jochen Topf <jochen@topf.org>.

  This file is part of OSMCoastline.

  OSMCoastline is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OSMCoastline is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OSMCoastline.  If not, see <https://www.gnu.org/licenses/>.

*/

enum return_codes {
    return_code_ok      = 0,
    return_code_warning = 1,
    return_code_error   = 2,
    return_code_fatal   = 3,
    return_code_cmdline = 4
};

#endif // OSMCOASTLINE_HPP
