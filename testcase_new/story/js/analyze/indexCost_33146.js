/******************************************************************************
 * @Description   : seqDB-33146:存在相同前缀的索引，查询条件为索引字段和非索引字段
 * @Author        : wuyan
 * @CreateTime    : 2022.08.30
 * @LastEditTime  : 2023.08.31
 * @LastEditors   : wu yan
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_index33146";
testConf.useSrcGroup = true;

main( test );
function test ()
{
   var cl = testPara.testCL;
   var groupName = testPara.srcGroupName;
   cl.createIndex( "ac", { "a": 1, "c": 1 } );
   cl.createIndex( "abc", { "a": 1, "b": 1, "c": 1 } );

   insertData( cl );
   db.analyze( { Collection: COMMCSNAME + "." + testConf.clName, SampleNum: 100 } );

   var masterDB = db.getRG( groupName ).getMaster().connect();
   var dbNodeIXStat = masterDB.getCS( "SYSSTAT" ).getCL( "SYSINDEXSTAT" );
   var statACCur = dbNodeIXStat.aggregate(
      { $match: { CollectionSpace: COMMCSNAME, Collection: testConf.clName, Index: "ac", "MCV.Values": { $expand: 1 } } },
      { $project: { "Key": "$MCV.Values.a" } },
      { $group: { _id: "$Key", Key: { $first: "$Key" }, Count: { $count: "$Key" } } },
      { $match: { Count: { $gt: 1 } } } )

   while( statACCur.next() )
   {
      var statRecord = statACCur.current().toObj()
      println( "---ab: " + JSON.stringify( statRecord ) )
      var statABCCur = dbNodeIXStat.aggregate(
         { $match: { CollectionSpace: COMMCSNAME, Collection: testConf.clName, Index: "abc", "MCV.Values": { $expand: 1 } } },
         { $project: { "Key": "$MCV.Values.a" } },
         { $group: { _id: "$Key", Key: { $first: "$Key" }, Count: { $count: "$Key" } } },
         { $match: { Key: { $et: statRecord["Key"] } } } )
      if( statABCCur.next() )
      {
         println( "---abc: " + JSON.stringify( statABCCur.current().toObj() ) )
         db.analyze( { Mode: 5, Collection: COMMCSNAME + "." + testConf.clName } )
         var indexName = cl.find( { a: statRecord["Key"], d: 10 } ).sort( { c: -1 } ).explain().current().toObj()["IndexName"];
         assert.equal( indexName, "ac", "use index is " + indexName + ", a = " + statRecord["Key"] );
      }
      statABCCur.close();
   }
   statACCur.close();
   masterDB.close();
}

function randomString ( maxLength )
{
   var arr = new Array( Math.round( Math.random() * maxLength ) )
   return arr.join( "a" )
}

function insertData ( dbcl )
{
   var batchSize = 10000;
   for( j = 0; j < 5; j++ ) 
   {
      data = [];
      for( i = 0; i < batchSize; i++ )
      {
         data.push( {
            a: Math.round( ( Math.random() * 80 ) ),
            b: randomString( 800 ),
            c: i,
            d: i
         } );
      }
      dbcl.insert( data );
   }
}