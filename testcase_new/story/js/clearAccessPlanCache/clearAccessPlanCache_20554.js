/***********************************************
*@description : seqDB-20554:$gt/$gte/$lt/$lte/$et查询优化
*@author      : XiaoNi Huang  2020-04-29 
************************************************/
testConf.skipStandAlone = true;
var testcaseID = 20554;
testConf.csName = CHANGEDPREFIX + "_cs_" + testcaseID;
testConf.csOpt = {};

main( testNormalCL, testMainSubCL );

function testNormalCL ()
{
   var clName = CHANGEDPREFIX + "_cl_" + testcaseID;
   var cl = commCreateCL( db, testConf.csName, clName );

   // create index
   cl.createIndex( "a", { "a": 1 } );
   cl.createIndex( "b", { "b": 1 } );
   cl.createIndex( "c", { "c": 1 } );

   // find.explain and check results
   // A: $gt/$gte/$lt/$lte, B: range, C: $et    A: 0.015 > B: 0.01> C: 0.005    select min
   // test: AB
   // $gt + range
   var rc = cl.find( { "a": { "$gt": 201910.0 }, "b": { "$gt": 201910.0, "$lt": 201911.0 } } ).explain();
   checkResults( rc, "ixscan", "b" );
   // $gte + range
   var rc = cl.find( { "a": { "$gte": 201910.0 }, "b": { "$gte": 201910.0, "$lt": 201911.0 } } ).explain();
   checkResults( rc, "ixscan", "b" );
   // $lt + range
   var rc = cl.find( { "a": { "$lt": 201911.0 }, "b": { "$gte": 201910.0, "$lt": 201911.0 } } ).explain();
   checkResults( rc, "ixscan", "b" );
   // $lte + range
   var rc = cl.find( { "a": { "$lte": 201911.0 }, "b": { "$gte": 201910.0, "$lte": 201911.0 } } ).explain();
   checkResults( rc, "ixscan", "b" );

   // test: BC
   // （range: $gt + $lt） + $et
   var rc = cl.find( { "b": { "$gt": 201910.0, "$lt": 201911.0 }, "c": 1 } ).explain();
   checkResults( rc, "ixscan", "c" );
   // （range: $gte + $lte） + $et
   var rc = cl.find( { "b": { "$gte": 201910.0, "$lte": 201911.0 }, "c": 1 } ).explain();
   checkResults( rc, "ixscan", "c" );

   // test: AC
   // $gt + range
   var rc = cl.find( { "a": { "$gt": 201910.0 }, "c": 1 } ).explain();
   checkResults( rc, "ixscan", "c" );

   // test: ABC
   var rc = cl.find( { "a": { "$gt": 201910.0 }, "b": { "$gte": 201910.0, "$lte": 201911.0 }, "c": 1 } ).explain();
   checkResults( rc, "ixscan", "c" );
}

function testMainSubCL ()
{
   var mclName = CHANGEDPREFIX + "_mcl_" + testcaseID;
   var sclNameBase = CHANGEDPREFIX + "_scl_" + testcaseID;

   // ready main-sub cl
   // create mainCL
   var mclOptions = { "ShardingKey": { "a": 1 }, "IsMainCL": true };
   var mcl = commCreateCL( db, testConf.csName, mclName, mclOptions, false );
   // createa subCL and index
   var sclOptions = { "ShardingKey": { "b": 1 }, "AutoSplit": true };
   for( i = 0; i < 5; i++ ) 
   {
      var cl = commCreateCL( db, testConf.csName, sclNameBase + "_" + i, sclOptions, false );
      cl.createIndex( 'c', { "c": 1 } );
   }
   // attach cl
   for( i = 0; i < 5; i++ ) 
   {
      var subCLFullName = testConf.csName + "." + sclNameBase + "_" + i;
      var lowBound = i * 100;
      var upBound = i * 100 + 100;
      mcl.attachCL( subCLFullName, { "LowBound": { "a": lowBound }, "UpBound": { "a": upBound } } );
   }

   // mainCL create index
   mcl.createIndex( 'a', { "a": 1 } );

   // find.explain and check results
   var rc = mcl.find( { "a": { "$gte": 10, "$lte": 200 }, "b": 1 } ).explain();
   checkResults( rc, "ixscan", "$shard", true );
}

function checkResults ( rc, expScanType, expIndexName, isMainCL )
{
   if( isMainCL == undefined ) { isMainCL = false; }
   var obj = {};
   if( !isMainCL )
   {
      obj = rc.current().toObj();
   }
   else
   {
      obj = rc.current().toObj().SubCollections[0];
   }

   if( expScanType !== obj.ScanType || expIndexName !== obj.IndexName )
   {
      throw new Error( "\nexpScanType: " + expScanType + ", expIndexName: " + expIndexName
         + "\nactScanType: " + obj.ScanType + ", atcIndexName: " + obj.IndexName
         + "\nexplain: " + JSON.stringify( obj ) );
   }
}
