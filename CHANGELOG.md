
# Change Log

All notable changes to this project will be documented in this file.
This project adheres to [Semantic Versioning](http://semver.org/).

## [unreleased] -

### Added

- Add spatialite scripts for creating grids for splitting.
- CMake config adds `clang-tidy` target.

### Changed

- Use `OGC_FID` instead of `ID` as id column in SQL scripts, that's how ogr
  expects it.
- Update to newest Protozero and Libosmium.
- Various small code-cleanup changes.

### Fixed

- Initialize stats with 0.
- `osmcoastline_ways`: Delete the copy and move constructor/assignment because
  we have a special destructor.
- Add `-pthread` compiler and linker options.


## [2.1.4] - 2016-09-16

### Changed

- Miscellaneous code cleanups.

### Fixed

- Windows build.


## [2.1.3] - 2016-03-30

### Added

- Add verbose option to `osmcoastline_filter`.
- `osmcoastline_filter` now shows memory used in verbose mode.

### Changed

- Optimized `osmcoastline_filter` program.
- Use more features from newest libosmium.

### Fixed

- Setting the sqlite output to unsynchronized speeds up writing to database.
- Now also works on GDAL 2. Fixes an error in the transaction handling.


## [2.1.2] - 2016-01-05

### Added

- Add --help/-h and --version/-V options to all programs.

### Changed

- Use a better approximation for the southernmost coordinate for Mercator
  projection.
- Updated for newest libosmium version (2.5.2).
- Uses gdalcpp.hpp from https://github.com/joto/gdalcpp instead of directly
  talking to GDAL/OGR. Makes this compatible with GDAL 2.
- Improved internal code using `unique_ptr` where possible.

### Fixed

- "Fixed" flag in rings layer now correct.


## [2.1.1] - 2015-08-31

### Changed

- Use newest libosmium release.


## [2.1.0] - 2015-08-18

### Added

- Optionally writes out list of all coastline segments and the new program
  `osmcoastline_segments` can compare those lists in various ways.

### Changed

- Updates for new libosmium version


## [2.0.1] - 2015-03-31

### Changed

- Added man pages


[unreleased]: https://github.com/osmcode/osmium-tool/compare/v2.1.4...HEAD
[2.1.4]: https://github.com/osmcode/osmium-tool/compare/v2.1.3...v2.1.4
[2.1.3]: https://github.com/osmcode/osmium-tool/compare/v2.1.2...v2.1.3
[2.1.2]: https://github.com/osmcode/osmium-tool/compare/v2.1.1...v2.1.2
[2.1.1]: https://github.com/osmcode/osmium-tool/compare/v2.1.0...v2.1.1
[2.1.0]: https://github.com/osmcode/osmium-tool/compare/v2.0.1...v2.1.0
[2.0.1]: https://github.com/osmcode/osmium-tool/compare/v2.0.0...v2.0.1

