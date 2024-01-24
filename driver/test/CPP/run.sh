#!/bin/sh

SCRIPT=$(readlink -f "$0")
INSTALL_DIR=`dirname "$SCRIPT"`
#echo $INSTALL_DIR

#$INSTALL_DIR/build_test/collection
#$INSTALL_DIR/build_test/collectionspace
#$INSTALL_DIR/build_test/cursor
#$INSTALL_DIR/build_test/sdb
#$INSTALL_DIR/build_test/cppbson
#$INSTALL_DIR/build_test/debug
#$INSTALL_DIR/build_test/domain
#$INSTALL_DIR/build_test/lob
#$INSTALL_DIR/build_test/procedure
#
#$INSTALL_DIR/build_test/split
#$INSTALL_DIR/build_test/rg


$INSTALL_DIR/build/dd/collection
$INSTALL_DIR/build/dd/collection.static
$INSTALL_DIR/build/dd/collectionspace
$INSTALL_DIR/build/dd/collectionspace.static
$INSTALL_DIR/build/dd/cppbson
$INSTALL_DIR/build/dd/cppbson.static
$INSTALL_DIR/build/dd/cursor
$INSTALL_DIR/build/dd/cursor.static
$INSTALL_DIR/build/dd/debug
$INSTALL_DIR/build/dd/debug.static
$INSTALL_DIR/build/dd/domain
$INSTALL_DIR/build/dd/domain.static
$INSTALL_DIR/build/dd/lob
$INSTALL_DIR/build/dd/lob.static
$INSTALL_DIR/build/dd/procedure
$INSTALL_DIR/build/dd/procedure.static
$INSTALL_DIR/build/dd/query
$INSTALL_DIR/build/dd/query.static
$INSTALL_DIR/build/dd/sdb
$INSTALL_DIR/build/dd/sdb.static
$INSTALL_DIR/build/dd/selectQuery
$INSTALL_DIR/build/dd/selectQuery.static
$INSTALL_DIR/build/dd/turnon_cache
$INSTALL_DIR/build/dd/turnon_cache.static
$INSTALL_DIR/build/dd/jira
$INSTALL_DIR/build/dd/jira.static
