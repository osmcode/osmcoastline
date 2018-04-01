------------------------------------------------------------------------------
--
-- Create grid for splitting up WGS84 polygons.
--
------------------------------------------------------------------------------

-- speed up spatialite processing
PRAGMA journal_mode = OFF;
PRAGMA synchronous = OFF;
PRAGMA temp_store = MEMORY;
PRAGMA cache_size = 5000000;

-- make sure we have the SRID in the spatial_ref_sys table
SELECT InsertEpsgSrid(4326) WHERE (SELECT count(*) FROM spatial_ref_sys WHERE srid = 4326) = 0;

-- clean up if this is left over from previous run
SELECT DiscardGeometryColumn('grid', 'geometry') FROM geometry_columns WHERE f_table_name='grid';
DROP TABLE IF EXISTS grid;

CREATE TABLE tmp_square_grid (id INTEGER PRIMARY KEY AUTOINCREMENT);
SELECT AddGeometryColumn('tmp_square_grid', 'geometry', 4326, 'MULTIPOLYGON', 'XY');

INSERT INTO tmp_square_grid (geometry)
    SELECT ST_SquareGrid(BuildMbr(-180.0, -90.0, 179.9999999999, 89.9999999, 4326), 1);

.elemgeo tmp_square_grid geometry tmp_square_grid_elem id_new id;

CREATE TABLE grid (id INTEGER PRIMARY KEY AUTOINCREMENT);
SELECT AddGeometryColumn('grid', 'geometry', 4326, 'POLYGON', 'XY');

INSERT INTO grid (geometry)
    SELECT ST_Intersection(
                -- the grid cell overlap has different width depending on
                -- latitude, in the other direction it is always 0.0005 degrees
                BuildMbr(ST_MinX(geometry) - (0.0005 / cos(radians(0.5 * (ST_MinY(geometry) + ST_MaxY(geometry))))),
                         ST_MinY(geometry) - 0.0005,
                         ST_MaxX(geometry) + (0.0005 / cos(radians(0.5 * (ST_MinY(geometry) + ST_MaxY(geometry))))),
                         ST_MaxY(geometry) + 0.0005, 4326),
                -- make sure there are no overlaps at the sides
                BuildMbr(-180.0, -90.0, 179.99999999, 89.99999999, 4326))
        FROM tmp_square_grid_elem;

SELECT CreateSpatialIndex('grid', 'geometry');

SELECT DiscardGeometryColumn('tmp_square_grid', 'geometry');
DROP TABLE tmp_square_grid;

SELECT DiscardGeometryColumn('tmp_square_grid_elem', 'geometry');
DROP TABLE tmp_square_grid_elem;

