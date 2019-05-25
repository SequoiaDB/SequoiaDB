#!/bin/sh

SCRIPT=$(readlink -f "$0")
INSTALL_DIR=`dirname "$SCRIPT"`
##echo $INSTALL_DIR
#$INSTALL_DIR/build_test/collection
#$INSTALL_DIR/build_test/collectionspace
#$INSTALL_DIR/build_test/cursor
#$INSTALL_DIR/build_test/sdb
##$INSTALL_DIR/build_test/snapshot
#$INSTALL_DIR/build_test/concurrent
#$INSTALL_DIR/build_test/shard
#$INSTALL_DIR/build_test/debug
#$INSTALL_DIR/build_test/cbson

$INSTALL_DIR/build/dd/bson_test
$INSTALL_DIR/build/dd/bson_test.static
$INSTALL_DIR/build/dd/cbson
$INSTALL_DIR/build/dd/cbson.static
$INSTALL_DIR/build/dd/collection
$INSTALL_DIR/build/dd/collection.static
$INSTALL_DIR/build/dd/collectionspace
$INSTALL_DIR/build/dd/collectionspace.static
$INSTALL_DIR/build/dd/concurrent
$INSTALL_DIR/build/dd/concurrent.static
$INSTALL_DIR/build/dd/cursor
$INSTALL_DIR/build/dd/cursor.static
#$INSTALL_DIR/build/dd/dc
#$INSTALL_DIR/build/dd/dc.static
$INSTALL_DIR/build/dd/debug
$INSTALL_DIR/build/dd/debug.static
$INSTALL_DIR/build/dd/domain
$INSTALL_DIR/build/dd/domain.static
$INSTALL_DIR/build/dd/lob
$INSTALL_DIR/build/dd/lob.static
#$INSTALL_DIR/build/dd/mutex
#$INSTALL_DIR/build/dd/mutex.static
#$INSTALL_DIR/build/dd/rg
#$INSTALL_DIR/build/dd/rg.static
$INSTALL_DIR/build/dd/sdb
$INSTALL_DIR/build/dd/sdb.static
$INSTALL_DIR/build/dd/selector
$INSTALL_DIR/build/dd/selector.static
#$INSTALL_DIR/build/dd/shard
#$INSTALL_DIR/build/dd/shard.static
$INSTALL_DIR/build/dd/snapshot
$INSTALL_DIR/build/dd/snapshot.static
$INSTALL_DIR/build/dd/turnon_cache
$INSTALL_DIR/build/dd/turnon_cache.static


