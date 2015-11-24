#ifndef UTIL_HPP
#define UTIL_HPP

/*

  Copyright 2012-2015 Jochen Topf <jochen@topf.org>.

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

#include <memory>
#include <type_traits>

template <typename R, typename T>
std::unique_ptr<R> make_unique_ptr_clone(const T* source) {
    static_assert(std::is_convertible<T*, R*>::value, "T* must be convertible to R*");
    return std::unique_ptr<R>(static_cast<R*>(source->clone()));
}

template <typename TDerived, typename TBase>
std::unique_ptr<TDerived> static_cast_unique_ptr(std::unique_ptr<TBase>&& ptr) {
    static_assert(std::is_base_of<TBase, TDerived>::value, "TDerived must be derived from TBase");
    return std::unique_ptr<TDerived>(static_cast<TDerived*>(ptr.release()));
}


#endif // UTIL_HPP
