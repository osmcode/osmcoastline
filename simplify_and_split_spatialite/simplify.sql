--
--  simplify.sql
--
--  You can use this to simplify coastline polygons if needed.
--  It will only work properly if polygons are not split and
--  the 3857 projection is used (ie if osmcoastline was run with
--  "-m 0 -s 3857").
--

CREATE TABLE simplified_land_polygons (
    OGC_FID INTEGER PRIMARY KEY
);

SELECT AddGeometryColumn('simplified_land_polygons', 'GEOMETRY', 3857, 'POLYGON', 'XY', 1);

--  Depending on the detail you need you might have to change the numbers here.
--  The given numbers work up to about zoom level 9.
INSERT INTO simplified_land_polygons (ogc_fid, geometry) SELECT ogc_fid, SimplifyPreserveTopology(geometry, 300) FROM land_polygons WHERE ST_Area(geometry) > 300000;

SELECT CreateSpatialIndex('simplified_land_polygons', 'GEOMETRY');

