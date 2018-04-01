-- ======================================================================
-- Create water polygons
--
-- Creates split water polygons from all split land polygons.
--
-- Call like this:
-- psql -d coastlines -f create_water_polygons.sql
--
-- ======================================================================

-- This is a helper view that makes the following INSERT easier to write.
-- For every tile this view contains the union of all land polygons in that tile.
CREATE VIEW merged_land_polygons AS
    SELECT tolerance, min_area, zoom, x, y, ST_Union(geom) AS geom
        FROM split_land_polygons p
        GROUP BY tolerance, min_area, zoom, x, y;

INSERT INTO split_water_polygons (tolerance, min_area, zoom, x, y, geom)
    SELECT p.tolerance, p.min_area, p.zoom, p.x, p.y, ST_Multi(ST_Difference(b.geom, p.geom))
        FROM merged_land_polygons p, bbox_tiles b
        WHERE p.zoom = b.zoom
          AND p.x = b.x
          AND p.y = b.y;

-- If there are no land polygons in a tile, no water polygon will be created for it.
-- This INSERT adds the water polygons in that case.
INSERT INTO split_water_polygons (tolerance, min_area, zoom, x, y, geom)
    SELECT p.tolerance, p.min_area, x, y, b.zoom, ST_Multi(geom)
        FROM bbox_tiles b, (SELECT distinct tolerance, min_area, zoom from split_land_polygons) p
        WHERE b.zoom=p.zoom
          AND x || '-' || y || '-' || b.zoom NOT IN (SELECT x || '-' || y || '-' || zoom FROM split_land_polygons);

DROP VIEW merged_land_polygons;

CREATE INDEX
    ON split_water_polygons
    USING GIST (geom);

