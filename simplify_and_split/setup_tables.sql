-- ======================================================================
-- Setup tables
-- ======================================================================

DROP TABLE IF EXISTS simplified_land_polygons;
CREATE TABLE simplified_land_polygons (
    id        SERIAL,
    fid       INT,
    tolerance INT,
    min_area  INT
);

SELECT AddGeometryColumn('simplified_land_polygons', 'geom', 3857, 'POLYGON', 2);


DROP TABLE IF EXISTS split_land_polygons;
CREATE TABLE split_land_polygons (
    id        SERIAL,
    fid       INT,
    tolerance INT,
    min_area  INT,
    zoom      INT,
    x         INT,
    y         INT
);

-- splitting can make multipolygons out of polygons, so geometry type is different
SELECT AddGeometryColumn('split_land_polygons', 'geom', 3857, 'MULTIPOLYGON', 2);


DROP TABLE IF EXISTS split_water_polygons;
CREATE TABLE split_water_polygons (
    id        SERIAL,
    tolerance INT,
    min_area  INT,
    zoom      INT,
    x         INT,
    y         INT
);

SELECT AddGeometryColumn('split_water_polygons', 'geom', 3857, 'MULTIPOLYGON', 2);


DROP TABLE IF EXISTS bbox_tiles;
CREATE TABLE bbox_tiles (
    id        SERIAL,
    zoom      INT,
    x         INT,
    y         INT
);

SELECT AddGeometryColumn('bbox_tiles', 'geom', 3857, 'POLYGON', 2);

CREATE INDEX idx_bbox_files_geom ON bbox_tiles USING GIST (geom);

