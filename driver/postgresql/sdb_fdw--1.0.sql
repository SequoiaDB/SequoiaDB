/* contrib/mongo_fdw/sdb_fdw--1.0.sql */

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION sdb_fdw" to load this file. \quit

CREATE FUNCTION sdb_fdw_handler()
RETURNS fdw_handler
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT;

CREATE FUNCTION sdb_fdw_validator(text[], oid)
RETURNS void
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT;

CREATE FOREIGN DATA WRAPPER sdb_fdw
  HANDLER sdb_fdw_handler
  VALIDATOR sdb_fdw_validator;
