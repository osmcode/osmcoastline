------------------------------------------------------------------------------
--
-- Create grid for splitting up Mercator polygons.
--
------------------------------------------------------------------------------

-- speed up spatialite processing
PRAGMA journal_mode = OFF;
PRAGMA synchronous = OFF;
PRAGMA temp_store = MEMORY;
PRAGMA cache_size = 5000000;

-- make sure we have the SRID in the spatial_ref_sys table
SELECT InsertEpsgSrid(3857) WHERE (SELECT count(*) FROM spatial_ref_sys WHERE srid = 3857) = 0;

-- clean up if this is left over from previous run
SELECT DiscardGeometryColumn('grid', 'geometry') FROM geometry_columns WHERE f_table_name='grid';
DROP TABLE IF EXISTS  grid;

CREATE TABLE tmp_square_grid (id INTEGER PRIMARY KEY AUTOINCREMENT);
SELECT AddGeometryColumn('tmp_square_grid', 'geometry', 3857, 'MULTIPOLYGON', 'XY');

INSERT INTO tmp_square_grid (geometry)
    SELECT ST_SquareGrid(
                -- slightly smaller than the full extent so we don't get an
                -- extra set of tiles at the top and right boundary
                BuildMbr(-20037508.34, -20037508.34, 20037508.0, 20037508.0, 3857),
                -- split into 64x64 tiles
                2*20037508.34/64);

.elemgeo tmp_square_grid geometry tmp_square_grid_elem id_new id;

CREATE TABLE grid (OGC_FID INTEGER PRIMARY KEY AUTOINCREMENT);
SELECT AddGeometryColumn('grid', 'geometry', 3857, 'POLYGON', 'XY');

INSERT INTO grid (geometry)
    SELECT ST_Intersection(
                -- 50 units overlap on each side of the tile
                ST_Expand(geometry, 50.0),
                -- make sure there are no overlaps at the sides
                BuildMbr(-20037508.34, -20037508.34, 20037508.34, 20037508.34, 3857))
        FROM tmp_square_grid_elem;

SELECT CreateSpatialIndex('grid', 'geometry');

SELECT DiscardGeometryColumn('tmp_square_grid', 'geometry');
DROP TABLE tmp_square_grid;

SELECT DiscardGeometryColumn('tmp_square_grid_elem', 'geometry');
DROP TABLE tmp_square_grid_elem;

