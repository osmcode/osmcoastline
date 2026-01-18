
# Change Log

All notable changes to this project will be documented in this file.
This project adheres to [Semantic Versioning](https://semver.org/).

## [unreleased] -

### Added

### Changed

- Switch from using text ids to 64bit integer ids.
- Various small code cleanups.

### Fixed

- Fix: Conversion of LinearRings into LineStrings.


## [2.4.1] - 2025-01-14

### Added

- Add option `-e, --exit-ignore-warnings`: Return with exit code 0 even if
  there are warnings.

### Changed

- Modernized code and CMake config, updates for new versions of libraries
  used.


## [2.4.0] - 2022-12-22

### Added

- `--format` option to `osmcoastline_filter` to set output file format.

### Changed

- Now needs at least C++14 and CMake 3.10.
- Various small code cleanups.

### Fixed

- Do not create empty polygons in water output.


## [2.3.1] - 2021-09-02

### Added

- GPKG output support

### Changed

- More tests, specifically for EPSG:3857.
- More error checks.
- Some shell script cleanups.

### Fixed

- Fix axis order problem with GDAL 3: In GDAL 3 the axis order for WGS84
  changed from (lon, lat) to (lat, lon)! So we need to use the magic "CRS84"
  instead which does the same thing in GDAL 2 and GDAL 3. This is an important
  fix, without it osmcoastline doesn't work with GDAL 3 when using any output
  SRS other than WGS84.

## [2.3.0] - 2021-01-08

### Added

- Add `-g`, `--gdal-driver=DRIVER` option to `osmcoastline`. This allows
  writing results to a shapefile or other format, not only to sqlite files.

### Changed

- Various small fixes and cleanups.
- Now depends on libosmium 2.16.0 or greater. This allows compiling with
  support for PBF lz4 compression which is enabled by default if the
  library is found. Disable by setting CMake option `WITH_LZ4` to `OFF`.

### Fixed

- Segfault in osmcoastline with newer GDAL versions (#39)


## [2.2.4] - 2019-02-27

### Changed

- Also look for newer clang-tidy versions in CMake config.

### Fixed

- Put Antarctic closure to exactly +/- 180 degree longitude.
- Add try/catch around most of main so we don't end with exception.


## [2.2.3] - 2019-02-06

### Fixed

- Compile with `NDEBUG` in `RelWithDebInfo` mode.
- Better error reporting on some exceptions.


## [2.2.2] - 2019-02-03

### Fixed

- Make `--output-lines` work even if `--output-polygons` is set to `none`.


## [2.2.1] - 2018-12-07

### Added

- We have now proper test cases. Just a few, but at least there is a framework
  for automated testing now.

### Changed

- Various small changes in the code and manuals to make it clearer.

### Fixed

- Various small bugs were fixed that lead to crashes in unusual circumstances.


## [2.2.0] - 2018-09-05

### Added

- Add spatialite scripts for creating grids for splitting.
- CMake config adds `clang-tidy` target.

### Changed

- Use `OGC_FID` instead of `ID` as id column in SQL scripts, that's how OGR
  expects it.
- Update to newer Protozero and Libosmium.
- Various small code-cleanup changes.
- Output extended version information on `--verbose` and `--version`.
- Derive exception used from `std::runtime_error`.
- Update to newest gdalcpp.

### Fixed

- Initialize stats with 0.
- `osmcoastline_ways`: Delete the copy and move constructor/assignment because
  we have a special destructor.
- Add `-pthread` compiler and linker options.
- Fix undefined behavior that resulted in more or less coastlines reported
  as "questionable".
- Lower right corner of Antarctica was being cut off in EPSG:3857.
- Very narrow water polygons were output near the anti-meridian in Antarctica.


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


[unreleased]: https://github.com/osmcode/osmium-tool/compare/v2.4.0...HEAD
[2.4.0]: https://github.com/osmcode/osmium-tool/compare/v2.3.1...v2.4.0
[2.3.1]: https://github.com/osmcode/osmium-tool/compare/v2.3.0...v2.3.1
[2.3.0]: https://github.com/osmcode/osmium-tool/compare/v2.2.4...v2.3.0
[2.2.4]: https://github.com/osmcode/osmium-tool/compare/v2.2.3...v2.2.4
[2.2.3]: https://github.com/osmcode/osmium-tool/compare/v2.2.2...v2.2.3
[2.2.2]: https://github.com/osmcode/osmium-tool/compare/v2.2.1...v2.2.2
[2.2.1]: https://github.com/osmcode/osmium-tool/compare/v2.2.0...v2.2.1
[2.2.0]: https://github.com/osmcode/osmium-tool/compare/v2.1.4...v2.2.0
[2.1.4]: https://github.com/osmcode/osmium-tool/compare/v2.1.3...v2.1.4
[2.1.3]: https://github.com/osmcode/osmium-tool/compare/v2.1.2...v2.1.3
[2.1.2]: https://github.com/osmcode/osmium-tool/compare/v2.1.1...v2.1.2
[2.1.1]: https://github.com/osmcode/osmium-tool/compare/v2.1.0...v2.1.1
[2.1.0]: https://github.com/osmcode/osmium-tool/compare/v2.0.1...v2.1.0
[2.0.1]: https://github.com/osmcode/osmium-tool/compare/v2.0.0...v2.0.1

