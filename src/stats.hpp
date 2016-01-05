#ifndef STATS_HPP
#define STATS_HPP

/*

  Copyright 2012-2016 Jochen Topf <jochen@topf.org>.

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
  along with OSMCoastline.  If not, see <http://www.gnu.org/licenses/>.

*/

struct Stats {
    unsigned int ways;
    unsigned int unconnected_nodes;
    unsigned int rings;
    unsigned int rings_from_single_way;
    unsigned int rings_fixed;
    unsigned int rings_turned_around;
    unsigned int land_polygons_before_split;
    unsigned int land_polygons_after_split;
};

#endif // STATS_HPP
