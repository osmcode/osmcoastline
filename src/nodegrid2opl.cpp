/*

  Copyright 2012-2021 Jochen Topf <jochen@topf.org>.

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

/**
 * This is a program used for testing only. It interprets an "ASCII art"
 * visualization of nodes in a grid read from STDIN and creates an OPL
 * output on STDOUT with those nodes.
 *
 * Nodes are represented by the characters 0 to 9 and a to z. All other
 * characters are ignored and are only used to form the grid. Here is
 * an example:
 *
 * ------------------
 *
 *  0   3    1
 *   \
 *    2--a-5
 *
 * ------------------
 *
 * This will result in something like:
 *
 * n100 v1 x1.030000 y1.990000
 * n101 v1 x1.120000 y1.990000
 * n102 v1 x1.050000 y1.970000
 * n103 v1 x1.070000 y1.990000
 * n105 v1 x1.100000 y1.970000
 * n110 v1 x1.080000 y1.970000
 *
 */

#include <algorithm>
#include <iostream>
#include <iterator>
#include <set>
#include <string>
#include <vector>

const int id_offset = 100;

static void add_node(std::vector<std::string>& nodes, char c, double x, double y) {
    static std::set<int> ids;
    int id = id_offset;

    if (c >= '0' && c <= '9') {
        id += c - '0';
    } else {
        id += c - 'a' + 10;
    }

    if (ids.count(id)) {
        std::cerr << "ID seen twice: " << c << " (" << id << ")\n";
        std::exit(1);
    }

    ids.insert(id);
    nodes.push_back("n" + std::to_string(id) + " v1 x" + std::to_string(x) + " y" + std::to_string(y) + "\n");
}

int main() {
    const double scale = 0.01;
    const double offset = 1;

    std::vector<std::string> nodes;

    int x = 1;
    int y = 100;

    for (std::string line; std::getline(std::cin, line);) {
        for (const auto c : line) {
            if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z')) {
                add_node(nodes, c, offset + x * scale, offset + y * scale);
            }
            ++x;
        }
        x = 1;
        --y;
    }

    std::sort(nodes.begin(), nodes.end());

    const auto it = std::ostream_iterator<std::string>(std::cout);
    std::copy(nodes.begin(), nodes.end(), it);

    return 0;
}

