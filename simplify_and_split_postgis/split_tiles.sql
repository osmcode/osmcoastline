-- ======================================================================
--
-- Split land polygon tiles into smaller tiles
--
-- Call like this:
-- psql -d $DB -v tolerance=$TOLERANCE -v min_area=$MIN_AREA -v from_zoom=$FROM_ZOOM -v to_zoom=$TO_ZOOM -f split_tiles.sql
--
-- for instance:
-- psql -d coastlines -v tolerance=0 -v min_area=0 -v from_zoom=5 -v to_zoom=6 -f split_tiles.sql
--
-- ======================================================================

INSERT INTO split_land_polygons (fid, tolerance, min_area, zoom, x, y, geom)
    SELECT p.fid, :tolerance, :min_area, :to_zoom, b.x, b.y, ST_Multi(ST_Intersection(p.geom, b.geom))
        FROM split_land_polygons p, bbox_tiles b
        WHERE ST_Intersects(p.geom, b.geom)
          AND p.tolerance=:tolerance
          AND p.min_area=:min_area
          AND p.zoom=:from_zoom
          AND b.zoom=:to_zoom
          AND ST_GeometryType(ST_Multi(ST_Intersection(p.geom, b.geom))) = 'ST_MultiPolygon';

