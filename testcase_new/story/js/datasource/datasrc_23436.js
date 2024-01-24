/******************************************************************************
 * @Description   : seqDB-23436:子表映射数据源，主表上查看访问计划
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2021.03.01
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_23436";
testConf.clOpt = { ShardingKey: { a: 1 }, ShardingType: "hash" };
testConf.useSrcGroup = true;
testConf.skipStandAlone = true;

main( test );
function test ( args )
{
   var sdbGroupName = args.srcGroupName;
   var dataSrcName = "datasrc23436";
   var csName = "cs_23436";
   var clName = "cl_23436";
   var srcCSName = "datasrcCS_23436";
   var mainCLName = "mainCL_23436";
   var indexName = "index_23436";

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   var datascrCL = commCreateCL( datasrcDB, srcCSName, clName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );

   var dbcs = db.createCS( csName );
   dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   var mainCL = commCreateCL( db, csName, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   mainCL.attachCL( COMMCSNAME + "." + COMMCLNAME + "_23436", { LowBound: { a: 0 }, UpBound: { a: 1000 } } );
   mainCL.attachCL( csName + "." + clName, { LowBound: { a: 1000 }, UpBound: { a: 2000 } } );
   datascrCL.createIndex( indexName, { a: 1 } );

   // 表扫描
   var docs = [];
   for( var i = 0; i < 2000; i++ )
   {
      var bValue = parseInt( Math.random() * 5000 );
      var cValue = parseInt( Math.random() * 5000 );
      docs.push( { a: i, b: bValue, c: cValue } );
   }
   mainCL.insert( docs );
   db.analyze( { Collection: csName + "." + mainCLName } );

   var cursor = mainCL.find().explain();
   var subClExplain = "";
   var clExplain = [];
   while( cursor.next() )
   {
      var SubCollections = cursor.current().toObj();
      if( SubCollections.Name == srcCSName + "." + clName )
      {
         clExplain = SubCollections;
      }
      else 
      {
         subClExplain = SubCollections;
      }
   }
   cursor.close();
   assert.equal( subClExplain.GroupName, sdbGroupName );
   var expectResult = datascrCL.find().explain();
   checkExplain( clExplain, expectResult );

   // 索引扫描
   var cursor = mainCL.find( { a: { $gt: 900 } } ).limit( 200 ).explain();
   while( cursor.next() )
   {
      var SubCollections = cursor.current().toObj();
      if( SubCollections.Name == srcCSName + "." + clName )
      {
         var clExplain = SubCollections;
      }
      else 
      {
         var subClExplain = SubCollections;
      }
   }
   cursor.close();
   assert.equal( subClExplain.GroupName, sdbGroupName );
   var expectResult = datascrCL.find( { a: { $gt: 900 } } ).limit( 200 ).explain();
   checkExplain( clExplain, expectResult );

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   datasrcDB.close();
}

function checkExplain ( actualResult, expectResult )
{
   assert.equal( actualResult.ScanType, expectResult.current().toObj().ScanType );
   assert.equal( actualResult.IndexName, expectResult.current().toObj().IndexName );
   assert.equal( actualResult.NodeName, expectResult.current().toObj().NodeName );
   expectResult.close();
}