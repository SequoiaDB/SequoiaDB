<?php
/*
 *  In PHP, single quotation are not the pass of the compiler analysis,
 *  but double quotes are compiler analysis.
 */

// create a new connection object
echo '<p>1. connect to server</p>' ;
$sdb = new SequoiaDB( 'localhost:11810' ) ;
$errno = $sdb->getError() ;
var_dump ( $errno ) ;

if ( $errno['errno'] != 0 )
{
   exit ;
}

// create a new collection space object
echo '<p>2. create collection space foo</p>' ;
$cs = $sdb->selectCS ( "foo" ) ;
var_dump ( $sdb->getError() ) ;
if( empty( $cs ) )
{
   exit ;
}

// create a new collection object
echo '<p>3. create collection test</p>' ;
$cl = $cs->selectCL ( "test" ) ;
if( empty( $cl ) )
{
   exit ;
}

// create a new list collection space cursor object
echo '<p>4. list collection spaces</p>' ;
$sdb_cursor = $sdb->listCS () ;
if ( $sdb_cursor )
{
   while ( $arr = $sdb_cursor->next () )
   {
      var_dump ( $arr ) ;
      echo "<br><br>" ;
   }
}

// create a new list collections cursor object
echo '<p>5. list collections</p>' ;
$sdb_cursor = $sdb->listCL () ;
if ( $sdb_cursor )
{
   // if you want to print sting,you can set install = false
   echo '<p>6. reset this cursor print mode string</p>' ;
   $sdb->install ( array ( 'install' => false ) ) ;
   while ( $str = $sdb_cursor->next () )
   {
      echo $str ;
      echo "<br><br>" ;
   }
}

// insert record array
echo '<p>7. insert array</p>' ;
$arr = array ( 'a' => 1, 'b' => new SequoiaDate('2012-12-21') ) ;
var_dump ( $cl->insert ( $arr ) ) ;
echo "<br><br>" ;

$str = '{"a":1,"b":{"$date":"2012-12-21"}}' ;
echo '<p>8. insert string '.$str.' </p>' ;
// insert record string
var_dump ( $cl->insert ( $str ) ) ;
echo "<br><br>" ;

// update all record
echo '<p>9. update record</p>' ;
var_dump ( $cl->update ( array ( '$set' => array ( 'c' => 'hello world' ) ) ) ) ;
echo "<br><br>" ;

// count from collection
echo '<p>10. count from collection</p>' ;
echo $cl->count () ;
echo " records<br><br>" ;

echo '<p>11. query from collection</p>' ;
// query from collection
$cl_cursor = $cl->find() ;
while ( $arr = $cl_cursor->next () )
{
   var_dump ( $arr ) ;
   echo "<br><br>" ;
}

$arr = array ( "a" => 1 ) ;
echo '<p>12. create index</p>' ;
// create index
var_dump ( $cl->createIndex ( $arr, 'myIndex' ) ) ;
echo "<br><br>" ;

echo '<p>13. query index</p>' ;
// query index
$index_cursor = $cl->getIndex ( 'myIndex' ) ;
while ( $arr = $index_cursor->getNext () )
{
   var_dump ( $arr ) ;
   echo "<br><br>" ;
}

echo '<p>14. delete index</p>' ;
// delete index
var_dump ( $cl->deleteIndex ( 'myIndex' ) ) ;
echo "<br><br>" ;

echo '<p>15. get name</p>' ;
echo 'collection space name: '. $cl->getCSName() ;
echo "<br><br>" ;
echo 'collection name: '. $cl->getCollectionName() ;
echo "<br><br>" ;
echo 'collection full name: '. $cl->getFullName() ;
echo "<br><br>" ;

$cl_cursor = $cl->find() ;
echo '<p>16. get next record</p>' ;
// get next record
$arr = $cl_cursor->getNext () ;
var_dump ( $arr ) ;
echo "<br><br>" ;
        

echo '<p>17. get current record</p>' ;
// get current record
$cl_cursor = $cl->find() ;  // The cursor does not point a record
$cl_cursor->getNext () ; // The cursor point the first record
$arr = $cl_cursor->current () ; // get the current point record
var_dump ( $arr ) ;
echo "<br><br>" ;


echo '<p>18. remove all record</p>' ;
// remove all record
var_dump ( $cl->remove() ) ;
echo "<br><br>" ;

echo '<p>19. delete collection</p>' ;
// delete collection
var_dump ( $cl->drop() ) ;
echo "<br><br>" ;

echo '<p>20. delete collection space</p>' ;
// delete collection
var_dump ( $cs->drop() ) ;
echo "<br><br>" ;

echo '<p>21. reset snapshot</p>' ;
// reset Snapshot
var_dump ( $sdb->resetSnapshot() ) ;
echo "<br><br>" ;

echo '<p>22. get snapshot</p>' ;
// reset Snapshot
$sdb_cursor = $sdb->getSnapshot( SDB_SNAP_DATABASE ) ;
if ( $sdb_cursor )
{
   while ( $arr = $sdb_cursor->next() )
   {
      var_dump ( $arr ) ;
      echo "<br><br>" ;
   }
}

echo '<p>23. get list</p>' ;
// reset Snapshot
$sdb_cursor = $sdb->getList( SDB_LIST_STORAGEUNITS ) ;
if ( $sdb_cursor )
{
   while ( $arr = $sdb_cursor->next() )
   {
      var_dump ( $arr ) ;
      echo "<br><br>" ;
   }
}

echo '<p>24. disconnection</p>' ;
// delete collection
$sdb->close() ;
echo "<br><br>" ;

?>