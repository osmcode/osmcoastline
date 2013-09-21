-- ======================================================================
--
-- Set up helper table with polygons containing the bbox for each tile
--
-- Call like this:
-- psql -d coastlines -v zoom=$ZOOM -f setup_bbox_tiles.sql
--
-- for instance:
-- psql -d coastlines -v zoom=3 -f setup_bbox_tiles.sql
--
-- ======================================================================

-- max value for x or y coordinate in spherical mercator
\set cmax 20037508.34
\set pow (2.0 ^ :zoom)::integer
\set tilesize (2*:cmax / :pow)

CREATE VIEW tiles AS SELECT * FROM generate_series(0, :pow - 1) AS x, generate_series(0, :pow - 1) AS y;
CREATE VIEW bbox AS SELECT :zoom AS zoom, x, y, x*:tilesize - :cmax AS x1, (:pow - y - 1)*:tilesize - :cmax AS y1, (x+1)*:tilesize - :cmax AS x2, (:pow - y)*:tilesize - :cmax AS y2 FROM tiles;

INSERT INTO bbox_tiles (zoom, x, y, geom)
    SELECT zoom, x, y, ST_SetSRID(ST_MakeBox2D(ST_MakePoint(x1, y1), ST_MakePoint(x2, y2)), 3857) AS geom
        FROM bbox;

DROP VIEW bbox;
DROP VIEW tiles;

