/***********************************************
*@description : seqDB-20556:无交集的查询条件优先走索引扫描
*@author      : XiaoNi Huang  2020-04-29 
************************************************/
testConf.clName = CHANGEDPREFIX + "_cs_20556";

main( test );
function test ( arg )
{
   var cl = arg.testCL;
   cl.createIndex( "a", { "a": 1 } );

   // first query
   var rc = cl.find( { "$and": [{ "a": 1 }, { "a": 2 }] } ).explain();
   checkResults( rc, "ixscan", "a", '{"a":[[{"$maxElement":1},{"$maxElement":1}]]}' );
   // second query
   var rc = cl.find( { "$and": [{ "a": 1 }, { "a": 3 }] } ).explain();
   checkResults( rc, "ixscan", "a", '{"a":[[{"$maxElement":1},{"$maxElement":1}]]}' );
   // third  query
   var rc = cl.find( { "$and": [{ "a": 1 }, { "a": 4 }] } ).explain();
   checkResults( rc, "ixscan", "a", '{"a":[[{"$maxElement":1},{"$maxElement":1}]]}' );
   // fourth query
   var rc = cl.find( { "$and": [{ "a": 1 }, { "a": 5 }] } ).explain();
   checkResults( rc, "ixscan", "a", '{"a":[[{"$maxElement":1},{"$maxElement":1}]]}' );
   // fifth query
   var rc = cl.find( { "$and": [{ "a": 1 }, { "a": 6 }] } ).explain();
   checkResults( rc, "ixscan", "a", '{"a":[[{"$maxElement":1},{"$maxElement":1}]]}' );
   // sixth query
   var rc = cl.find( { "$and": [{ "a": 1 }, { "a": 7 }] } ).explain();
   checkResults( rc, "ixscan", "a", '{"a":[[{"$maxElement":1},{"$maxElement":1}]]}' );

   var rc = cl.find( { "$and": [{ "a": 1 }, { "a": 1 }] } ).explain();
   checkResults( rc, "ixscan", "a", '{"a":[[1,1]]}' );
}

function checkResults ( rc, expScanType, expIndexName, expIXBound )
{
   var obj = rc.current().toObj();
   if( expScanType !== obj.ScanType || expIndexName !== obj.IndexName
      || expIXBound !== JSON.stringify( obj.IXBound ) )
   {
      throw new Error( "\nexpScanType: " + expScanType + ", expIndexName: " + expIndexName + ", expIXBound: " + expIXBound
         + "\nactScanType: " + obj.ScanType + ", atcIndexName: " + obj.IndexName + ", actIXBound: " + JSON.stringify( obj.IXBound )
         + "\nexplain: " + JSON.stringify( obj ) );
   }
}
