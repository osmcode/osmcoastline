
# Change Log

All notable changes to this project will be documented in this file.
This project adheres to [Semantic Versioning](http://semver.org/).

## [unreleased] -

### Added

- Add --help/-h and --version/-V options to all programs.

### Changed

- Use a better approximation for the southernmost coordinate for Mercator
  projection.
- Updated for newest libosmium version (2.5.2).
- Uses gdalcpp.hpp from https://github.com/joto/gdalcpp instead of directly
  talking to GDAL/OGR. Makes this compatible with GDAL 2.
- Improved internal code using unique_ptr where possible.

### Fixed


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


[unreleased]: https://github.com/osmcode/osmium-tool/compare/v2.1.1...HEAD
[2.1.1]: https://github.com/osmcode/osmium-tool/compare/v2.1.0...v2.1.1
[2.1.0]: https://github.com/osmcode/osmium-tool/compare/v2.0.1...v2.1.0
[2.0.1]: https://github.com/osmcode/osmium-tool/compare/v2.0.0...v2.0.1

