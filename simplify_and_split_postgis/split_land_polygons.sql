-- ======================================================================
--
-- Split land polygons into tiles
--
-- This can either split non-simplified land polygons or simplified land
-- polygons. Set tolerance=0 and min_area=0 to use non-simplified land
-- polygons, otherwise the simplified polygons with the given parameters
-- are used.
--
-- Call like this:
-- psql -d $DB -v tolerance=$TOLERANCE -v min_area=$MIN_AREA -v zoom=$ZOOM -f split_land_polygons.sql
--
-- for instance:
-- psql -d coastlines -v tolerance=300 -v min_area=300000 -v zoom=3 -f split_land_polygons.sql
--
-- ======================================================================

-- Only one of the following INSERT statements will do anything thanks to the
-- last condition (:tolerance (!)=0).

-- case 1: split non-simplified polygons
INSERT INTO split_land_polygons (fid, tolerance, min_area, zoom, x, y, geom)
    SELECT p.fid, 0, 0, b.zoom, b.x, b.y, ST_Multi(ST_Intersection(p.geom, b.geom))
        FROM land_polygons p, bbox_tiles b
        WHERE p.geom && b.geom
          AND ST_Intersects(p.geom, b.geom)
          AND b.zoom=:zoom
          AND ST_GeometryType(ST_Multi(ST_Intersection(p.geom, b.geom))) = 'ST_MultiPolygon'
          AND :tolerance=0;

-- case 2: split simplified polygons
INSERT INTO split_land_polygons (fid, tolerance, min_area, zoom, x, y, geom)
    SELECT p.fid, p.tolerance, p.min_area, b.zoom, b.x, b.y, ST_Multi(ST_Intersection(p.geom, b.geom))
        FROM simplified_land_polygons p, bbox_tiles b
        WHERE p.geom && b.geom
          AND ST_Intersects(p.geom, b.geom)
          AND p.tolerance=:tolerance
          AND p.min_area=:min_area
          AND b.zoom=:zoom
          AND ST_GeometryType(ST_Multi(ST_Intersection(p.geom, b.geom))) = 'ST_MultiPolygon'
          AND :tolerance!=0;

CREATE INDEX
    ON split_land_polygons
    USING GIST (geom)
    WHERE tolerance=:tolerance
        AND min_area=:min_area
        AND zoom=:zoom;

