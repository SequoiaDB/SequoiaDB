/******************************************************************************
 * @Description   : seqDB-24371:创建/删除多个本地索引    
 * @Author        : wu yan
 * @CreateTime    : 2021.09.23
 * @LastEditTime  : 2022.01.29
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clOpt = { ReplSize: -1 };
testConf.clName = COMMCLNAME + "_standaloneIndex_24371";

main( test );
function test ( testPara )
{
   var recordNum = 1000;
   var maxIndexNum = 64;
   var dbcl = testPara.testCL;
   var fullclName = COMMCSNAME + "." + testConf.clName;

   var nodeName = getCLOneNodeName( db, fullclName );
   insertDatas( dbcl, recordNum, maxIndexNum );
   var indexNames = createIndexes( dbcl, nodeName, maxIndexNum );

   checkStandaloneIndexTask( "Create index", fullclName, nodeName, indexNames );
   checkExplainUseStandAloneIndex( COMMCSNAME, testConf.clName, nodeName, { "test0": 10, "test1": 10 }, "ixscan", indexNames[0] );
   listIndexesAndCheckIndexName( COMMCSNAME, testConf.clName, nodeName, indexNames );

   //删除本地索引
   deleteIndexs( dbcl, indexNames );
   checkStandaloneIndexTask( "Drop index", fullclName, nodeName, indexNames );
   var actIndexNames = [];
   listIndexesAndCheckIndexName( COMMCSNAME, testConf.clName, nodeName, actIndexNames );
}

function insertDatas ( dbcl, recordNum, maxIndexNum )
{
   var doc = [];
   for( var k = 0; k < recordNum; k++ )
   {
      var a = {};
      for( var i = 0; i < maxIndexNum; i++ )
      {
         a["test" + i] = "testindex_" + i + "_no_" + k;
      }
      doc.push( a );
   }
   dbcl.insert( doc )
}

function createIndexes ( dbcl, nodeName, maxIndexNum )
{
   var indexNames = [];
   //默认存在id索引，最多创建64-1=63个索引   
   for( var i = 0; i < maxIndexNum - 1; i++ )
   {
      var indexDef = {};
      var subIndexName = "Index_24371_" + i;
      indexDef["test" + i] = 1;
      dbcl.createIndex( subIndexName, indexDef, { Standalone: true }, { NodeName: nodeName } );
      indexNames.push( subIndexName );
   }
   return indexNames;
}

function deleteIndexs ( dbcl, indexNames )
{
   for( var i = 0; i < indexNames.length; i++ )
   {
      var indexName = indexNames[i];
      dbcl.dropIndex( indexName );
   }
}

function listIndexesAndCheckIndexName ( csName, clName, nodeName, indexNames )
{
   var dataDB = new Sdb( nodeName );
   var dbcl = dataDB.getCS( csName ).getCL( clName );
   var cursor = dbcl.listIndexes();
   var actIndexNames = [];
   while( cursor.next() )
   {
      var obj = cursor.current().toObj().IndexDef;
      var name = obj.name;
      var isStandalone = obj.Standalone;
      //排除默认创建的$id索引
      if( name !== "$id" )
      {
         actIndexNames.push( name );
         assert.equal( isStandalone, true, name + "  should be standalone index!" )
      }
   }
   cursor.close();
   dataDB.close();
   actIndexNames.sort();
   indexNames.sort();
   assert.equal( actIndexNames, indexNames );
}