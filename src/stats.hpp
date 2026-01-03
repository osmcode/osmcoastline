#ifndef STATS_HPP
#define STATS_HPP

/*

  Copyright 2012-2026 Jochen Topf <jochen@topf.org>.

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

struct Stats {
    unsigned int ways = 0;
    unsigned int unconnected_nodes = 0;
    unsigned int rings = 0;
    unsigned int rings_from_single_way = 0;
    unsigned int rings_fixed = 0;
    unsigned int rings_turned_around = 0;
    unsigned int land_polygons_before_split = 0;
    unsigned int land_polygons_after_split = 0;
};

#endif // STATS_HPP
